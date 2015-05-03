/*
 * TWMapImagesRecovery is a tool for extract png images from teeworlds maps.
 *
 * Usage:
 *    <application bin> [-options] <map> <destination folder>
 *
 * Options:
 *    --version			Shows Version
 *    -v				Verbose Mode
 *    -i				Only shows the info of a map (Version, Author, License, etc..)... use with (-v)
 *    -r				Only recovery the images
 *
 * Default:
 *   By default the application starts in SILENCE mode.
 */
#include "datafile.h"
#include "system.h"
#include <pnglite.h>
#include <iostream>
#define APP_VERSION		"1.6"

enum {
	MODE_SILENCE = 0,
	MODE_VERBOSE,

	OPTION_INFO = 1<<1,
	OPTION_STEAL = 1<<2,
};
bool stealIt(const char *pathMap, const char *pathSave, int options);

short g_Mode = MODE_SILENCE;

int main(int argc, char* argv[])
{
	int options = 0;
	char fileMap[512]={0};
	char destFolder[512]={0};

	if (argc == 2)
	{
		if (strcmp(argv[1], "--version") == 0)
			std::cout << "TWMapImagesRecovery "APP_VERSION << std::endl;
		else
			std::cout << "Parameter Unknown!" << std::endl;

		return 0;
	}
	else if (argc == 4)
	{
		std::string params = argv[1];

		strncpy(fileMap, argv[2], sizeof(fileMap));
		strncpy(destFolder, argv[3], sizeof(destFolder));

		if (params.at(0) == '-')
		{
			if (params.find("v") != std::string::npos) g_Mode = MODE_VERBOSE;
			if (params.find("i") != std::string::npos) options |= OPTION_INFO;
			if (params.find("r") != std::string::npos) options |= OPTION_STEAL;
		}
		else
		{
			std::cout << "ERROR: Invalid Parameters!" << std::endl;
			return -1;
		}
	}
	else if (argc != 3)
	{
		std::string appName = argv[0];
		appName = appName.substr(appName.find_last_of("\\/")+1).c_str();

		std::cout << "ERROR: Map or destination folder don't specified!" << std::endl << std::endl;
		std::cout << "Usage: " << appName << " [-v] <map> <destination folder>" << std::endl;
		return -1;
	}
	else
	{
		strncpy(fileMap, argv[1], sizeof(fileMap));
		strncpy(destFolder, argv[2], sizeof(destFolder));
	}

	if (!fs_is_file(fileMap))
	{
		std::cout << "ERROR: The map doesn't exists!" << std::endl << std::endl;
		return -1;
	}

	if (!fs_is_dir(destFolder))
	{
		std::cout << "ERROR: The folder destination doesn't exists!" << std::endl << std::endl;
		return -1;
	}

	png_init(0, 0);

	return stealIt(fileMap, destFolder, options)?0:1;
}

bool stealIt(const char *pathMap, const char *pathSave, int options)
{
	CDataFileReader fileReader;

	if(!fileReader.open(pathMap))
	{
		std::cout << "ERROR: The file isn't a valid map!" << std::endl << std::endl;
		return false;
	}


	// check version
	CMapItemVersion *pItem = (CMapItemVersion *)fileReader.findItem(MAPITEMTYPE_VERSION, 0);
	if(pItem && pItem->m_Version == 1)
	{
		// load map info
		if ((options&OPTION_INFO) || options == OPTION_INFO || (!options && g_Mode == MODE_VERBOSE))
		{
			std::cout << "MAP INFO" << std::endl << "====================" << std::endl;

			CMapItemInfo *pItem = (CMapItemInfo *)fileReader.findItem(MAPITEMTYPE_INFO, 0);

			std::cout << "Author: ";
			if(pItem->m_Author > -1) std::cout << (char *)fileReader.getData(pItem->m_Author) << std::endl;
			else std::cout << "-Unsaved!-" << std::endl;

			std::cout << "Version: ";
			if(pItem->m_MapVersion > -1) std::cout << (char *)fileReader.getData(pItem->m_MapVersion) << std::endl;
			else std::cout << "-Unsaved!-" << std::endl;

			std::cout << "Credits: ";
			if(pItem->m_Credits > -1) std::cout << (char *)fileReader.getData(pItem->m_Credits) << std::endl;
			else std::cout << "-Unsaved!-" << std::endl;

			std::cout << "License: ";
			if(pItem->m_License > -1) std::cout << (char *)fileReader.getData(pItem->m_License) << std::endl;
			else std::cout << "-Unsaved!-" << std::endl;

			std::cout << "====================" << std::endl;
		}

		// load images
		if (!options || (options&OPTION_STEAL))
		{
			int Start, Num;
			fileReader.getType( MAPITEMTYPE_IMAGE, &Start, &Num);

			if (g_Mode == MODE_VERBOSE)
				std::cout << std::endl << "SCANNING " << Num << " IMAGES..." << std::endl << "==========================" << std::endl;

			for(int i = 0; i < Num; i++)
			{
				CMapItemImage *pItem = (CMapItemImage *)fileReader.getItem(Start+i, 0, 0);
				char *pName = (char *)fileReader.getData(pItem->m_ImageName);

				if(pItem->m_External)
				{
					if (g_Mode == MODE_VERBOSE)
						std::cout << "[EXTERNAL] " << pName << "\t\t\tOMITTED" << std::endl;
				}
				else
				{
					char finalPath[512]={0};
					snprintf(finalPath, sizeof(finalPath), "%s/%s.png", pathSave, pName);

					// copy image data
					png_t png;
					png_open_file_write(&png, finalPath);
					png_set_data(&png, pItem->m_Width, pItem->m_Height, 8, PNG_TRUECOLOR_ALPHA, (unsigned char*)fileReader.getData(pItem->m_ImageData));
					png_close_file(&png);

					if (g_Mode == MODE_VERBOSE)
						std::cout << "[INTERNAL] " << pName << "\t\t\tEXTRACTED!" << std::endl;
				}
			}

			if (g_Mode == MODE_VERBOSE)
				std::cout << "==========================" << std::endl;
		}
	}

	return fileReader.close();
}
