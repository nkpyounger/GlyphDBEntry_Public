/* Natalie Younger, 2015
 * Program to read image files within hierarchical file structure
 * and enter into SQL table.
 *
 * Error checking to add:
 * Need to check array length when copying chars to imgAttributes members
 * ???
 *
 * To make SQL connector work with CodeBlocks:
 * MySQL
 * Project > Build Options > Linker Settings
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\lib\libmariadb.lib
 * Project > Build Options > Search Directories (Compiler and Linker)
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\lib
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\include
 *
 * List of includes required for basic functionality:
 * #include <stdio.h>
 * #include <windows.h>
 * #include <mysql.h>
 * But NOT my_global.h? (messes up ALL KINDS of definitions)
 *
 * Must initialize connection pointer
 */

#include <winsock2.h>

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include <iostream>
#include <string>
#include <sstream>
#include <string.h>

#include <mysql.h>

using namespace std;

struct imgAttributes
/* Struct to collect file attributes for entry in SQL table
 */
{
    char col1[5]          	= {0};
    char col2[50]        	= {0};
    char col3[100]      	= {0};
    char col4[10]           = {0};
    char col5[50]    		= {0};
    char fullPath[MAX_PATH] = {0};
};

void setPath(char *newPath, char *origPath, int maxLength, char *extension);
int countFiles(char *currentPath, HANDLE h, WIN32_FIND_DATA *findData);
void populateArray(char *currentPath, char *fileList [], int arraySize, HANDLE h,
                   WIN32_FIND_DATA *findData);
void printArray(char *fileList [], int arraySize);
void freeMem(char *fileList [], int arraySize);
void printImgAttributes(imgAttributes *imgfile);
void setCol23Labels(imgAttributes *imgfile, char *LvlXDir);
void setCol4(imgAttributes *imgfile, char *LvlYDir);
void setImgPath(imgAttributes *imgfile, int maxLength, char *currentPath);
void setCol5(imgAttributes *imgfile, char *fileName);
void setZeroAttributes(imgAttributes *imgfile);
void finish_with_error(MYSQL *con);

int main()
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char basePath[] = "C:\\Users\\USER\\Documents\\Full Database";
    char currentPath[MAX_PATH] = {0}; 	//For tracking current location within
										// file structure
    char tempPath[MAX_PATH] = {0};	//For direct use with "FindFile" functions
    setPath(currentPath, basePath, MAX_PATH, TEXT(""));
    StringCchCopy(tempPath, MAX_PATH, currentPath);
    StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

    // Find number of folders(Level 1) in the Full Database Directory
    // includes "."(current location) and ".." (up a level)
    int numLvl1Dirs = 0;
    numLvl1Dirs = countFiles(tempPath, hFind, &ffd);
    char *Lvl1Array [numLvl1Dirs - 2] = {0};
    populateArray(tempPath, Lvl1Array, numLvl1Dirs, hFind, &ffd);

    // Change to first Level 1 directory
    setPath(currentPath, basePath, MAX_PATH, Lvl1Array[0]);
    StringCchCopy(tempPath, MAX_PATH, currentPath);
    StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

    // Find number of folders in its subdirectory (Level 2)
    int numLvl2Dirs = 0;
    numLvl2Dirs = countFiles(tempPath, hFind, &ffd);
    char *Lvl2Array [numLvl2Dirs - 2] = {0};
    populateArray(tempPath, Lvl2Array, numLvl2Dirs, hFind, &ffd);

    // Connect to MySQL
    MYSQL *con = mysql_init(NULL);

    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, "localhost", "USER", "PASSWORD",
          "DATABASE_NAME", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

    // Begin loop down to image files
    setPath(currentPath, basePath, MAX_PATH, TEXT(""));
    StringCchCopy(tempPath, MAX_PATH, currentPath);
    StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));
	
	// Temporary struct for organizing info for DB entry
	// Overwritten after every loop
    imgAttributes testFile;
	
	// Track number of files already in database
	// Assuming program is run on the same file structure multiple times
	// after adding more image files, it avoids re-adding files already present in DB
    int numDBmatches = 0;
    int dbcount = 0;

    for(int i = 0; i < (numLvl1Dirs - 2); i++)
    {

        for(int j = 0; j < (numLvl2Dirs - 2); j++)
        {
            setPath(currentPath, basePath, MAX_PATH, Lvl1Array[i]);
            setPath(currentPath, currentPath, MAX_PATH, Lvl2Array[j]);
            StringCchCopy(tempPath, MAX_PATH, currentPath);
            StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

            int k = 0;
            hFind = FindFirstFile(tempPath, &ffd);
            char imagePath[MAX_PATH] = {0};
            do
            {
                if(k < 2)
                {
                    k++;
                }
                else
                {

                    setPath(imagePath, currentPath, MAX_PATH, ffd.cFileName);
                    setCol23Labels(&testFile, Lvl1Array[i]);
                    setCol4(&testFile, Lvl2Array[j]);
                    setImgPath(&testFile, MAX_PATH, imagePath);
                    setCol5(&testFile, ffd.cFileName);

                    FILE *fp = fopen(testFile.fullPath, "rb");
                    if (fp == NULL)
                    {
                        fprintf(stderr, "cannot open image file\n");
                        exit(1);
                    }

                    fseek(fp, 0, SEEK_END);

                    if (ferror(fp))
                    {
                        fprintf(stderr, "fseek() failed\n");
                        int r = fclose(fp);

                        if (r == EOF)
                        {
                            fprintf(stderr, "cannot close file handler\n");
                        }

                        exit(1);
                    }

                    int flen = ftell(fp);

                    if (flen == -1)
                    {
                        perror("error occurred");
                        int r = fclose(fp);

                        if (r == EOF)
                        {
                            fprintf(stderr, "cannot close file handler\n");
                        }

                        exit(1);
                    }

                    fseek(fp, 0, SEEK_SET);

                    if (ferror(fp))
                    {
                        fprintf(stderr, "fseek() failed\n");
                        int r = fclose(fp);

                        if (r == EOF)
                        {
                            fprintf(stderr, "cannot close file handler\n");
                        }

                        exit(1);
                    }

                    char data[flen+1];

                    int data_size = fread(data, 1, flen, fp);

                    if (ferror(fp))
                    {
                        fprintf(stderr, "fread() failed\n");
                        int r = fclose(fp);

                        if (r == EOF)
                        {
                            fprintf(stderr, "cannot close file handler\n");
                        }

                        exit(1);
                    }

                    int r = fclose(fp);

                    if (r == EOF)
                    {
                        fprintf(stderr, "cannot close file handler\n");
                    }

                    char *chunk = new char [2*data_size+1];
                    mysql_real_escape_string(con, chunk, data, data_size);

                    size_t 1_len = strlen(testFile.col1);
                    size_t 2_len = strlen(testFile.col2);
                    size_t 3_len = strlen(testFile.col3);
                    size_t 4_len = strlen(testFile.col4);
                    size_t 5_len = strlen(testFile.col5);

                    char st2[] = "SELECT * from table WHERE Col1 = '%s' AND Col5 = '%s' AND Col4 = '%s'";
                    size_t st2_len = strlen(st2);
                    char *query2 = new char [st2_len + id_len + fname_len + 4_len];
                    int len2 = snprintf(query2, (st2_len + id_len + fname_len + 4_len), st2, testFile.col1, testFile.col5, testFile.col4);

                    if (mysql_query (con, query2)) // If this statement executes without error:
                    {                              // DO NOT execute this block
                        finish_with_error(con); // Check if this file already in table
                    }

                    MYSQL_RES *result = mysql_store_result(con);
                    MYSQL_ROW row;
                    row = mysql_fetch_row(result);
                    if (row) //If data is found in rows
                    {
                        numDBmatches++;
                    }
                    else if (!row)
                    {
                        char *st = "INSERT INTO table (Col1, Col2, Col3, Col4, Col5, Img) VALUES('%s', '%s', '%s', '%s', '%s', '%s')";
                        size_t st_len = strlen(st);
                        int tot_len = st_len + (2*data_size+1) + 1_len + 2_len +
                                            3_len + 4_len + 5_len;
                        char *query = new char [tot_len];
                        //*query2 = {0};
                        int len = snprintf(query, tot_len, st, testFile.col1, testFile.col2,
                                    testFile.col3, testFile.col4, testFile.col5, chunk);
                        if (mysql_real_query (con, query, len)) // If you DO find duplicates:
                        {										// execute this block
                            finish_with_error(con);
                        }
                        delete[] query;
                    }
                    else
                    {
                        std::cout << "oh dear, something's gone very very wrong...\n";
                        exit (1);
                    }
                    delete[] chunk; delete[] query2;
                    mysql_free_result(result);

                    // Blank out the struct
					setZeroAttributes(&testFile);

                    k++;
                    dbcount++;
					// Keep track that the program is still running
                    if (!(dbcount%50))
                    {
                        cout << "Read " << dbcount << " files.\n";
                    }
                }
            }
            while (FindNextFile(hFind, &ffd) != 0);
        }
    }

    // Delete allocated memory
    mysql_close(con);
    freeMem(Lvl1Array, (numLvl1Dirs - 2));
    freeMem(Lvl2Array, (numLvl2Dirs - 2));
    cout << "Number of Database entries matched: " << numDBmatches << "\n";
    return 0;
}

void setPath(char *newPath, char *origPath, int maxLength, char *extension)
{
    // Check that (base path + subdirectory extension + \...\*\0 (4 char))
    // does not exceed allocated string length
    // Although all 4 char may not be appended, check worst case scenario
    if((strlen(origPath) + strlen(extension) + 4) >
       static_cast<unsigned int>(maxLength))
    {
        cout << "Path too long\n";
        exit (1);
    }

    StringCchCopy(newPath, maxLength, origPath); // Set base path

    if (strlen(extension)) // Add extension to path
    {
        StringCchCat(newPath, maxLength, TEXT("\\"));
        StringCchCat(newPath, maxLength, extension);
    }
}

int countFiles(char *currentPath, HANDLE h, WIN32_FIND_DATA *findData)
{
    h = FindFirstFile(currentPath, findData);
    int numFiles = 0;
    do
    {
        numFiles++;
    }
    while (FindNextFile(h, findData) != 0);

    return numFiles;
}

void populateArray(char *currentPath, char *fileList [], int arraySize, HANDLE h,
                   WIN32_FIND_DATA *findData)
{
    h = FindFirstFile(currentPath, findData);
    int i = 0;
    do
    {
        if(i < 2)
        {
            i++;
            continue;
        }
        else if (i >= arraySize)
        {
            cout << "Out of bounds error.\n";
            exit (1);
        }
        else
        {
            try
            {
                fileList[i - 2] = new char [MAX_PATH];
            }
            catch(std::bad_alloc&)
            {
                cout << "Memory allocation failed.\n";
                exit(1);
            }
            StringCchCopy(fileList[i - 2], MAX_PATH, findData->cFileName);
            i++;
        }
    }
    while (FindNextFile(h, findData) != 0);
}

void printArray(char *fileList [], int arraySize)
{
    for(int i = 0; i < arraySize; i++)
    {
        cout << fileList[i] << "\n";
    }
}

void freeMem(char *fileList [], int arraySize)
{
    for(int i = 0; i < arraySize; i++)
    {
        delete[] fileList[i];
    }
}

void printImgAttributes(imgAttributes *imgfile)
{
    cout << "Image file attributes for database: \n";
    cout << "Col4 Value: "     		<< imgfile->col4 		<< "\n";
    cout << "Location on Disk: "    << imgfile->fullPath    << "\n";
    cout << "Col1 Value: "       	<< imgfile->col1      	<< "\n";
    cout << "Col2 Value: "   		<< imgfile->col2     	<< "\n";
    cout << "Col3 Value: "          << imgfile->col3    	<< "\n";
    cout << "Col4 Value: "         	<< imgfile->col4        << "\n";
}

void setCol23Labels (imgAttributes *imgfile, char *LvlXDir)
{
    int i = 0;
    while(LvlXDir[i] != '-') // Parse dir name "AAA-AAAA"
    {
        if(i == 0)
        {
            switch (LvlXDir[i])
            {
				// Switch statement to encode info not directly provided in
				// directory names
                case 'A': StringCchCopy(imgfile->col2, 50, "Ripe"); break;
                case 'B': StringCchCopy(imgfile->col2, 50, "Rotten"); break;
				//...etc.
                default: cout << "Incorrect Category Label\n"; exit (1);
            }
        }
        imgfile->col2[i] = LvlXDir[i];
        i++;
    }
    i++; // skip "-"
    int fix = i; // adjust following indexing
    while(LvlXDir[i])
    {
        imgfile->col3[i - fix] = LvlXDir[i];
        i++;
    }
}

void setCol4(imgAttributes *imgfile, char *LvlYDir)
{
    int i = 0;
    while(LvlYDir[i])
    {
        imgfile->col4[i] = LvlYDir[i];
        i++;
    }
}

void setImgPath(imgAttributes *imgfile, int maxLength, char *currentPath)
{
    StringCchCopy(imgfile->fullPath, maxLength, currentPath);
}

void setCol5(imgAttributes *imgfile, char *fileName)
{
    int i = 0;
    while(fileName[i])
    {
        imgfile->col5[i] = fileName[i];
        i++;
    }
}

void setZeroAttributes(imgAttributes *imgfile)
{
    fill(imgfile->col1, (imgfile->col1 + sizeof(imgfile->col1)), 0);
    fill(imgfile->col2, (imgfile->col2 + sizeof(imgfile->col2)), 0);
    fill(imgfile->col3, (imgfile->col3 + sizeof(imgfile->col3)), 0);
    fill(imgfile->col4, (imgfile->col4 + sizeof(imgfile->col4)), 0);
    fill(imgfile->col5, (imgfile->col5 + sizeof(imgfile->col5)), 0);
    fill(imgfile->fullPath, (imgfile->fullPath + sizeof(imgfile->fullPath)), 0);
}

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}
