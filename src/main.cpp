#include "datafile.h"
#include "system.h"
#include <pnglite.h>
#include <iostream>
#define APP_VERSION		"1.5"

bool stealIt(const char *pathMap, const char *pathSave);

int main(int argc, char* argv[])
{
	if (argc == 2)
	{
		if (strcmp(argv[1], "-v") == 0)
			std::cout << "TWMapImagesRecovery "APP_VERSION << std::endl;
		else
			std::cout << "Parameter Unknown!" << std::endl;

		return 0;
	}
	else if (argc != 3)
	{
		std::string appName = argv[0];
		appName = appName.substr(appName.find_last_of("\\/")+1).c_str();

		std::cout << "TWMapImagesRecovery "APP_VERSION << std::endl << "----------------- ----- -- -" << std::endl;
		std::cout << "ERROR: Map or destination folder don't specified!" << std::endl << std::endl;
		std::cout << "Usage: " << appName << " [-v] <map> <destination folder>" << std::endl;
		return -1;
	}

	if (!fs_is_file(argv[1]))
	{
		std::cout << "TWMapImagesRecovery "APP_VERSION << std::endl << "----------------- ----- -- -" << std::endl;
		std::cout << "ERROR: The map doesn't exists!" << std::endl << std::endl;
		return -1;
	}

	if (!fs_is_dir(argv[2]))
	{
		std::cout << "TWMapImagesRecovery "APP_VERSION << std::endl << "----------------- ----- -- -" << std::endl;
		std::cout << "ERROR: The folder destination doesn't exists!" << std::endl << std::endl;
		return -1;
	}

	png_init(0, 0);
	stealIt(argv[1], argv[2]);

	return 0;
}

bool stealIt(const char *pathMap, const char *pathSave)
{
	CDataFileReader fileReader;

	if(!fileReader.open(pathMap))
		return 0;


	// check version
	CMapItemVersion *pItem = (CMapItemVersion *)fileReader.findItem(MAPITEMTYPE_VERSION, 0);
	if(pItem && pItem->m_Version == 1)
	{
		std::cout << std::endl << std::endl;

		// load map info
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

		std::cout << std::endl;

		// load images
		{
			int Start, Num;
			fileReader.getType( MAPITEMTYPE_IMAGE, &Start, &Num);

			std::cout << "SCANNING " << Num << " IMAGES..." << std::endl << "==========================" << std::endl;

			for(int i = 0; i < Num; i++)
			{
				CMapItemImage *pItem = (CMapItemImage *)fileReader.getItem(Start+i, 0, 0);
				char *pName = (char *)fileReader.getData(pItem->m_ImageName);

				if(pItem->m_External)
					std::cout << "[EXTERNAL] " << pName << "\t\t\tOMITTED" << std::endl;
				else
				{
					char finalPath[512]={0};
					snprintf(finalPath, sizeof(finalPath), "%s/%s.png", pathSave, pName);

					// copy image data
					png_t png;
					png_open_file_write(&png, finalPath);
					png_set_data(&png, pItem->m_Width, pItem->m_Height, 8, PNG_TRUECOLOR_ALPHA, (unsigned char*)fileReader.getData(pItem->m_ImageData));
					png_close_file(&png);

					std::cout << "[INTERNAL] " << pName << "\t\t\tEXTRACTED!" << std::endl;
				}
			}

			std::cout << "==========================" << std::endl;
		}
	}

	return fileReader.close();
}
