#include <algorithm>
#include <SDL_image.h>
#include "ZE_Core.h"
#include "tinyxml2.h"
#include "ZE_AssetManager.h"

using namespace std;
using namespace tinyxml2;

void AssetManager::Init(string xmlPath)
{
	XMLDocument doc;
	if (XML_NO_ERROR != doc.LoadFile(xmlPath.c_str()))
	{
		throw std::runtime_error("XMLDocument LoadFile ERROR");
	}
	XMLElement* docroot = doc.RootElement();
	XMLElement* sub = docroot->FirstChildElement("element");
	while (sub)
	{
		string type;
		string name;
		string path;
		string xml;

		const XMLAttribute* attr = sub->FirstAttribute();
		type = attr->Value();
		attr = attr->Next();
		name = attr->Value();
		attr = attr->Next();
		path = attr->Value();
		attr = attr->Next();
		xml = attr->Value();

		if (type == "image")
			LoadTexture(name, path, xml);
		else if (type == "ttf")
			LoadTTF(name, path);
		else if (type == "music" || type == "sound")
		{
			bool isMusic = false;
			if (type == "music")
				isMusic = true;
			LoadSound(isMusic, name, path);
		}

		sub = sub->NextSiblingElement();
	}
}

textureStruct AssetManager::getTexture(string name)
{
	textureStruct temp;
	for (unsigned int i = 0; i < TEXTURES.size(); i++)
	{
		if (name == TEXTURES[i]->name)
		{
			temp.subData = TEXTURES[i]->mySubInfo;
			temp.texture = TEXTURES[i];
			return temp;
			break;
		}
		for (unsigned int j = 0; j < TEXTURES[i]->subTextures.size(); j++)
		{
			if (name == TEXTURES[i]->subTextures[j].name)
			{
				temp.subData = TEXTURES[i]->subTextures[j];
				temp.texture = TEXTURES[i];
				return temp;
				break;
			}
		}
	}
	ZE_error.PopDebugConsole_Error("Can't find Texture: " + name);
	return temp;
}

SDL_Texture* AssetManager::getTTFTexture(string text, string name, int size, SDL_Color color, int* width, int* height,
	unsigned int effectLevel)
{
	auto usingfont = getFont(name);
	TTF_Font* tempfont = usingfont->getFont(size);
	SDL_Surface* tempsur = NULL;
	if (effectLevel == 0)
		tempsur = TTF_RenderUTF8_Solid(tempfont, text.c_str(), color);
	else if (effectLevel == 1)
		tempsur = TTF_RenderUTF8_Shaded(tempfont, text.c_str(), color, { 0, 0, 0 });
	else if (effectLevel == 2)
		tempsur = TTF_RenderUTF8_Blended(tempfont, text.c_str(), color);
	if (tempsur == NULL)
	{
		ZE_error.PopDebugConsole_SDL_ttfError("Unable to render text surface!");
		return NULL;
	}
	else
	{
		SDL_Texture* temptexture = Surface2SDLTexture(tempsur, width, height);
		return temptexture;
	}
}

shared_ptr<Font> AssetManager::getFont(string name)
{
	for (auto i : FONT)
	{
		if (name == i->name)
		{
			return i;
		}
	}
	ZE_error.PopDebugConsole_Error("Can't find font name:" + name);
	return nullptr;
}

shared_ptr<Sound> AssetManager::getSound(string name)
{
	for (auto i : SOUNDS)
	{
		if (name == i->name)
		{
			return i;
		}
	}
	ZE_error.PopDebugConsole_Error("Can't find sound name:" + name);
	return nullptr;
}

//用于对比两个结构体的大小，供getTextures排序使用，若A小于B将返回true
//因为sort似乎不能访问类里面的函数，所以写外面了
bool comparTextureStruce(textureStruct a, textureStruct b)
{
	return a.subData.name < b.subData.name;
}

deque<textureStruct> AssetManager::getTextures(string partOfName)
{
	deque<textureStruct> temp;
	for (unsigned int i = 0; i < TEXTURES.size(); i++)
	{
		textureStruct oneOfTemp;
		if (TEXTURES[i]->name.find(partOfName) == 0)
		{
			oneOfTemp.subData = TEXTURES[i]->mySubInfo;
			oneOfTemp.texture = TEXTURES[i];
			temp.push_back(oneOfTemp);
		}
		else
		{
			for (unsigned int j = 0; j < TEXTURES[i]->subTextures.size(); j++)
			{
				if (TEXTURES[i]->subTextures[j].name.find(partOfName) == 0)
				{
					oneOfTemp.subData = TEXTURES[i]->subTextures[j];
					oneOfTemp.texture = TEXTURES[i];
					temp.push_back(oneOfTemp);
				}
			}
		}
	}
	if (temp.size() == 0)
	{
		ZE_error.PopDebugConsole_Error("Can't find Texture has part of name: " + partOfName);
	}
	sort(temp.begin(), temp.end(), comparTextureStruce);
	return temp;
}

void AssetManager::LoadTexture(string name, string path, string xml)
{
	/*之前，TEXTURES是一个一般对象数组而不是指针数组。
	这个方法会首先声明一个局部Texture变量，Init后，push进TEXTURE数组里
	但这样贴图会丢失。
	我猜想，这是因为局部变量Texture在方法结束后即被销毁，所以push进TEXTURE的元素是一个类似野指针的存在。
	这可能也是为什么大家都用指针的原因，就好像你在A类里盖了一栋楼，你想让B看看，总不能说把楼搬过去给B看。
	所以就拿一个指针，告诉B地址让他自己来看就好了。*/

	auto temp_texture = make_shared<Texture>();

	//总之宽高必须从SURFACE读取，所以声明两个变量，让S2T方法用指针传回来
	//直接把函数放在INIT里的话，无法正确传递WH变量，所以单独拿出来赋值一遍。
	int tempWidth = 0;
	int tempHeight = 0;

	//声明一个子贴图数组，用于传递给贴图类
	deque <SubTexture> subtexture = SubXmlReader(xml);

	//调用下面的读取地址方法获得一个surface指针传递给S2T，
	SDL_Texture* tempTexture = Surface2SDLTexture(ImageReader(path), &tempWidth, &tempHeight);
	// 初始化
	temp_texture->Init(name, tempTexture, tempWidth, tempHeight, subtexture);
	// 保存指针
	TEXTURES.push_back(temp_texture);
}

void AssetManager::LoadTTF(string name, string path)
{
	FONT.emplace_back(new Font(name, path));
}

void AssetManager::LoadSound(bool isMusic, string name, string path)
{
	if (isMusic)
	{
		Mix_Music* music = Mix_LoadMUS(path.c_str());
		SOUNDS.emplace_back(new Sound(name, music));
	}
	else
	{
		Mix_Chunk* sound = Mix_LoadWAV(path.c_str());
		SOUNDS.emplace_back(new Sound(name, sound));
	}
}

SDL_Texture* AssetManager::Surface2SDLTexture(SDL_Surface* surface, int* getW, int* getH)
{
	SDL_Texture* newTexture = NULL;
	newTexture = SDL_CreateTextureFromSurface(g_ZE_MainRenderer, surface);
	//将Surface转换为Texture
	//直接创建贴图也行，但是要的参数复杂很多，不如直接转换
	if (newTexture == NULL)
	{
		ZE_error.PopDebugConsole_SDLError("Unable to creat texture!");
	}
	//获取宽高，用指针传回去
	*getW = surface->w;
	*getH = surface->h;
	SDL_FreeSurface(surface);
	//将临时的Surface释放掉，时刻记得，没用了立即扔，免得秋后算账
	//这是因为上面那个格式化方法会复制一个surface而不是指向原来的地址，毕竟占用的内存体积可能不一样
	return newTexture;
}

SDL_Surface* AssetManager::ImageReader(string path, int extNameLength)
{
	string temp;
	for (int i = 0; i < extNameLength; i++)
	{
		//循环读取字符串的最后三位赋值给temp，extNameLength默认为3。
		//头文件中的函数声明设置了默认参数这里就不能再设置了，记住，不然会跳一个C2572错误
		temp += path[path.length() - (extNameLength - i)];
		//最后temp会变成一个扩展名
	}

	SDL_Surface* tempSurface = NULL;

	if (temp == "bmp" || temp == "BMP")
	{
		//载入BMP的方法使用SDL原生载入函数，SDL只能载入BMP
		tempSurface = SDL_LoadBMP(path.c_str());
		//因为是纯C，不接受string变量，不再赘述
		if (tempSurface == NULL)
		{
			ZE_error.PopDebugConsole_SDLError("Unable to load bmp file:" + path + " |Load error");
		}
	}
	else if (temp == "png" || temp == "PNG")
	{
		//SDL只能载入BMP，PNG就用SDL_IMAGE
		tempSurface = IMG_Load(path.c_str());
		if (tempSurface == NULL)
		{
			ZE_error.PopDebugConsole_SDL_ImageError("Unable to load png file:" + path + " |Load error");
		}
	}
	else
	{
		ZE_error.PopDebugConsole_Error("Unable to read " + temp + " file.");
		//if们都没截住那就是没搞的格式呗...
	}

	return tempSurface;
}

deque<SubTexture> AssetManager::SubXmlReader(string xml)
{
	deque<SubTexture> temp;
	if (xml == "")
	{
		return temp;
	}
	else
	{
		XMLDocument doc;
		doc.LoadFile(xml.c_str());
		XMLElement* resourses = doc.RootElement();
		XMLElement* texture = resourses->FirstChildElement("SubTexture");
		while (texture)
		{
			SubTexture tempSub;
			const XMLAttribute* attr = texture->FirstAttribute();
			tempSub.name = attr->Value();
			attr = attr->Next();
			tempSub.x = attr->IntValue();
			attr = attr->Next();
			tempSub.y = attr->IntValue();
			attr = attr->Next();
			tempSub.width = attr->IntValue();
			attr = attr->Next();
			tempSub.height = attr->IntValue();
			attr = attr->Next();
			if (attr == NULL)
			{
				tempSub.frameX = 0;
				tempSub.frameY = 0;
				tempSub.frameWidth = tempSub.width;
				tempSub.frameHeight = tempSub.height;
			}
			else
			{
				tempSub.frameX = attr->IntValue();
				attr = attr->Next();
				tempSub.frameY = attr->IntValue();
				attr = attr->Next();
				tempSub.frameWidth = attr->IntValue();
				attr = attr->Next();
				tempSub.frameHeight = attr->IntValue();
			}
			temp.push_back(tempSub);
			texture = texture->NextSiblingElement();
		}
		return temp;
	}
}

AssetManager::~AssetManager()
{
	dispose();
}

void AssetManager::dispose()
{
	DeleteAllTextures();
	DeleteAllFonts();
	DeleteAllSounds();
}

void AssetManager::DeleteSound(int index)
{
	// 智能指针化
	//SOUNDS[index]->~Sound();
	//delete(SOUNDS[index]);
	//SOUNDS[index] = NULL;
	SOUNDS.erase(SOUNDS.begin() + index);
}

void AssetManager::DeleteAllSounds()
{
	while (SOUNDS.size() != 0)
	{
		DeleteSound(0);
	}
	SOUNDS.clear();
}

void AssetManager::DeleteFont(int index)
{
	// 智能指针化
	//FONT[index]->~Font();
	//delete(FONT[index]);
	//FONT[index] = NULL;
	FONT.erase(FONT.begin() + index);
}

void AssetManager::DeleteAllFonts()
{
	while (FONT.size() != 0)
	{
		DeleteFont(0);
	}
	FONT.clear();
}

void AssetManager::DeleteTexture(int index)
{
	// 智能指针化
	//TEXTURES[index]->~Texture();
	//delete(TEXTURES[index]);
	//TEXTURES[index] = NULL;
	//删除数组元素
	TEXTURES.erase(TEXTURES.begin() + index);
}

void AssetManager::DeleteAllTextures()
{
	//AllTexture是一个指针vector
	while (TEXTURES.size() != 0)
	{
		DeleteTexture(0);
		//永远记得，不要野指针，不要内存泄露
	}

	/*我要羞耻一下你，以前这个东西你是这么写的，简化过了
	for(var i = 0; i < TEXTURES.size(); i++)
	{
		dispose(TEXTURES[i])
		remove(TEXTURES[i])
	}
	我以为只有上学的时候才会犯这种傻逼错误。
	首先你不断的删除数组元素，SIZE越来越小，I越来越大，那删一半不就退出循环了吗
	然后你还删的是i，卧槽，数组越来越小，有上一半I就指向非法数据了
	这尼玛导致的内存泄露DEBUG了快两天
	别再傻逼了*/

	TEXTURES.clear();
}