/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* Modified by unsigned char* */
#ifndef ENGINE_SHARED_DATAFILE_H
#define ENGINE_SHARED_DATAFILE_H
#include <fstream>

enum
{
	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE
};

struct CMapItemVersion
{
	int m_Version;
};

struct CMapItemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
};

struct CMapItemImage
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
};

struct CDatafileItemType
{
	int m_Type;
	int m_Start;
	int m_Num;
} ;

struct CDatafileItem
{
	int m_TypeAndID;
	int m_Size;
};

struct CDatafileHeader
{
	char m_aID[4];
	int m_Version;
	int m_Size;
	int m_Swaplen;
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
};

struct CDatafileData
{
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
	char m_aStart[4];
};

struct CDatafileInfo
{
	CDatafileItemType *m_pItemTypes;
	int *m_pItemOffsets;
	int *m_pDataOffsets;
	int *m_pDataSizes;

	char *m_pItemStart;
	char *m_pDataStart;
};

struct CDatafile
{
	std::ifstream *m_pFile;
	unsigned m_Crc;
	CDatafileInfo m_Info;
	CDatafileHeader m_Header;
	int m_DataStartOffset;
	char **m_ppDataPtrs;
	char *m_pData;
};

// raw datafile access
class CDataFileReader
{
	struct CDatafile *m_pDataFile;
	void *getDataImpl(int Index, int Swap);
public:
	CDataFileReader() : m_pDataFile(0) {}
	~CDataFileReader() { close(); }

	bool isOpen() const { return m_pDataFile != 0; }

	bool open(const char *pFilename);
	bool close();

	void *getData(int Index);
	int getDataSize(int Index);
	long getDataSizeUncompressed(int Index);
	void unloadData(int Index);
	void *getItem(int Index, int *pType, int *pID);
	void *getDataSwapped(int Index);
	int getItemSize(int Index);
	void getType(int Type, int *pStart, int *pNum);
	void *findItem(int Type, int ID);
	int numItems();
	int numData();
	void unload();

	unsigned crc();
};

#endif
