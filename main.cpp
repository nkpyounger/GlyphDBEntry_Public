/* Extension of Testing9 - Add struct for image file info
 * Read test set of Image file names into array and parse into struct
 *
 * Functionality to read folders in sign list directory,
 * switch to a subdirectory and read all source names into
 * second array
 * Added some error handling for memory allocation
 *
 * Error checking to add:
 * Need to check array length when copying chars to imgAttributes members
 *
 * MySQL
 * Project > Build Options > Linker Settings
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\lib\libmariadb.lib
 * Project > Build Options > Search Directories (Compiler and Linker)
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\lib
 *      C:\Program Files\MariaDB 10.0\MariaDB Connector C 64-bit\include
 * Must include:
 * #include <stdio.h>
 * #include <windows.h>
 * #include <mysql.h>
 * AND my_global.h? - screws up ALL KINDS of definitions
 *
 * Must initialize connection pointer


 TO FIX: WEIRD AUTOINCREMENT, JPGS SAVED AS PNGS (OPEN WITH IRFAN) - unexpected matches from
 M17 double reed leaf = m17 (single reed leaf) - rename double folder to M17x

 Can reset autoincrement in Heidi by: SELECT MAX( `autoincr_column` ) FROM `table` ; ALTER TABLE `table` AUTO_INCREMENT = number;
 where "number" is the maxvalue +1 (if delete all, just reset to 0)
 */

//#include "my_global.h"

#include <winsock2.h> //due to warning thrown up by including my_global.h below (just above mysql.h)

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include <iostream>
#include <string>
#include <sstream>
//highest location my_global.h doesn't cause cascade of errors
#include <string.h>

//#include "changeDir2.h"
//#include "listFiles.h"

//#include <typeinfo>
//#include <my_global.h>
#include <mysql.h>

using namespace std;

struct imgAttributes
{
    char gardID[5]          = {0};
    char gardCat[50]        = {0};
    char signName[100]      = {0};
    char src[10]            = {0};
    char sysFileName[50]    = {0};
    char fullPath[MAX_PATH] = {0};
};

void setPath(char *newPath, char *origPath, int maxLength, char *extension);
int countFiles(char *currentPath, HANDLE h, WIN32_FIND_DATA *findData);
void populateArray(char *currentPath, char *fileList [], int arraySize, HANDLE h,
                   WIN32_FIND_DATA *findData);
void printArray(char *fileList [], int arraySize);
void freeMem(char *fileList [], int arraySize);
void printImgAttributes(imgAttributes *imgfile);
void setGardinerLabels(imgAttributes *imgfile, char *signNameDir);
void setSource(imgAttributes *imgfile, char *srcNameDir);
void setImgPath(imgAttributes *imgfile, int maxLength, char *currentPath);
void setSysName(imgAttributes *imgfile, char *fileName);
void setZeroAttributes(imgAttributes *imgfile);
void finish_with_error(MYSQL *con);

int main()
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char basePath[] =
        "C:\\Users\\USER\\Documents\\Handwritten Hieroglyphs\\Sign Database";
    // Set up arrays containing list
    // Set path to upper level directories (containing sign names and codes)
    char currentPath[MAX_PATH] = {0};
    char tempPath[MAX_PATH] = {0};
    setPath(currentPath, basePath, MAX_PATH, TEXT(""));
    StringCchCopy(tempPath, MAX_PATH, currentPath);
    StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

    // Find number of folders in the Sign Database Directory
    // (including "." and "..")
    int numSignDirs = 0;
    numSignDirs = countFiles(tempPath, hFind, &ffd);

    char *signListArray [numSignDirs - 2] = {0};
    populateArray(tempPath, signListArray, numSignDirs, hFind, &ffd);
    //cout << "SIGN LIST\n";
    //printArray(signListArray, (numSignDirs - 2));

    // Change to first subdirectory
    setPath(currentPath, basePath, MAX_PATH, signListArray[0]);
    StringCchCopy(tempPath, MAX_PATH, currentPath);
    StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

    // Find number of folders in Sources Subdirectory
    int numSrcDirs = 0;
    numSrcDirs = countFiles(tempPath, hFind, &ffd);

    char *srcListArray [numSrcDirs - 2] = {0};
    populateArray(tempPath, srcListArray, numSrcDirs, hFind, &ffd);
    //cout << "\nSOURCES\n";
    //printArray(srcListArray, (numSrcDirs - 2));
    // end setup of signListArray and srcListArray - functionalize this?

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

    imgAttributes testFile;

    int numDBmatches = 0;
    int dbcount = 0;

    for(int i = 0; i < (numSignDirs - 2); i++)
    {
//        setPath(currentPath, basePath, MAX_PATH, signListArray[i]);
//        StringCchCopy(tempPath, MAX_PATH, currentPath);
//        StringCchCat(tempPath, MAX_PATH, TEXT("\\*"));

        for(int j = 0; j < (numSrcDirs - 2); j++)
        {
            setPath(currentPath, basePath, MAX_PATH, signListArray[i]);
            setPath(currentPath, currentPath, MAX_PATH, srcListArray[j]);
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
                    //continue;
                }
                else
                {

                    setPath(imagePath, currentPath, MAX_PATH, ffd.cFileName);
                    setGardinerLabels(&testFile, signListArray[i]);
                    setSource(&testFile, srcListArray[j]);
                    setImgPath(&testFile, MAX_PATH, imagePath);
                    setSysName(&testFile, ffd.cFileName);

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

                    size_t id_len = strlen(testFile.gardID);
                    size_t cat_len = strlen(testFile.gardCat);
                    size_t sname_len = strlen(testFile.signName);
                    size_t src_len = strlen(testFile.src);
                    size_t fname_len = strlen(testFile.sysFileName);

                    char st2[] = "SELECT * from sign_list WHERE GardID = '%s' AND Sys_FileName = '%s' AND Src = '%s'";
                    size_t st2_len = strlen(st2);
                    char *query2 = new char [st2_len + id_len + fname_len + src_len];
                    //*query2 = {0};
                    int len2 = snprintf(query2, (st2_len + id_len + fname_len + src_len), st2, testFile.gardID,
                                            testFile.sysFileName, testFile.src);

                    if (mysql_query (con, query2)) //means if this statement executes without error:
                    {                               //DO NOT execute this block
                        finish_with_error(con); //check if this file already in table
                    }

                    MYSQL_RES *result = mysql_store_result(con);
                    MYSQL_ROW row;
                    row = mysql_fetch_row(result);
                    if (row) //if data in rows
                    {
                        //std::cout << "found a match!\n";
                        //printImgAttributes(&testFile);
                        //cout << "\n";
                        numDBmatches++;
                    }
                    else if (!row)
                    {
                        //std::cout << "no matches!\n";
                        char *st = "INSERT INTO sign_list (GardID, Gard_Cat, Sign_Name, Src, Sys_FileName, Img) VALUES('%s', '%s', '%s', '%s', '%s', '%s')";
                        size_t st_len = strlen(st);
                        int tot_len = st_len + (2*data_size+1) + id_len + cat_len +
                                            sname_len + src_len + fname_len;
                        char *query = new char [tot_len];
                        //*query2 = {0};
                        int len = snprintf(query, tot_len, st, testFile.gardID, testFile.gardCat,
                                    testFile.signName, testFile.src, testFile.sysFileName, chunk);
                        if (mysql_real_query (con, query, len)) //means if you DO find duplicates:
                        {
                            //std::cout << "did this work?\n";        //execute this block
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

//                    if(!(k%100))
//                    {
//                        cout << "\nIMAGE ATTRIBUTES\n";
//                        printImgAttributes(&testFile);
//                    }

                    setZeroAttributes(&testFile);

                    k++;
                    dbcount++;
                    if (!(dbcount%50))
                    {
                        cout << "Read " << dbcount << " files.\n";
                    }

//                    char userInput = '0';
//                    while(userInput != 'y' && userInput != 'n')
//                    {
//                        cout << "Continue? (y/n) ";
//                        cin >> userInput;
//                        if(userInput == 'n') exit(1);
//                    }
                }
            }
            while (FindNextFile(hFind, &ffd) != 0);
        }
    }

    // Delete allocated memory
    mysql_close(con);
    freeMem(signListArray, (numSignDirs - 2));
    freeMem(srcListArray, (numSrcDirs - 2));
//    freeMem(imgArray, (numImages - 2));
    cout << "Number of Database entries matched: " << numDBmatches << "\n";
    cout << "Hello world!" << endl;
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

    //StringCchCat(newPath, maxLength, TEXT("\\*")); // Add termination
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
    cout << "Image File Name: "     << imgfile->sysFileName << "\n";
    cout << "Location on Disk: "    << imgfile->fullPath    << "\n";
    cout << "Gardiner Code: "       << imgfile->gardID      << "\n";
    cout << "Gardiner Category: "   << imgfile->gardCat     << "\n";
    cout << "Sign Name: "           << imgfile->signName    << "\n";
    cout << "Source Text: "         << imgfile->src         << "\n";
}

void setGardinerLabels (imgAttributes *imgfile, char *signNameDir)
{
    int i = 0;
    while(signNameDir[i] != '-')
    {
        if(i == 0) //if !j?
        {
            switch (signNameDir[i])
            {
                case 'A':
                {
                    if(signNameDir[i+1] == 'a')
                        StringCchCopy(imgfile->gardCat, 50, "Unclassified");
                    else
                        StringCchCopy(imgfile->gardCat, 50, "Man_and_his_Occupations");
                    break;
                }
                case 'B': StringCchCopy(imgfile->gardCat, 50, "Woman_and_her_Occupations"); break;
                case 'C': StringCchCopy(imgfile->gardCat, 50, "Anthropomorphic_Deities"); break;
                case 'D': StringCchCopy(imgfile->gardCat, 50, "Parts_of_the_Human_Body"); break;
                case 'E': StringCchCopy(imgfile->gardCat, 50, "Mammals"); break;
                case 'F': StringCchCopy(imgfile->gardCat, 50, "Parts_of_Mammals"); break;
                case 'G': StringCchCopy(imgfile->gardCat, 50, "Birds"); break;
                case 'H': StringCchCopy(imgfile->gardCat, 50, "Parts_of_Birds"); break;
                case 'I': StringCchCopy(imgfile->gardCat, 50, "Amphibious_Animals,_Reptiles,_etc."); break;
                case 'K': StringCchCopy(imgfile->gardCat, 50, "Fish_and_Parts_of_Fish"); break;
                case 'L': StringCchCopy(imgfile->gardCat, 50, "Invertebrates_and_Lesser_Animals"); break;
                case 'M': StringCchCopy(imgfile->gardCat, 50, "Trees_and_Plants"); break;
                case 'N': StringCchCopy(imgfile->gardCat, 50, "Sky,_Earth,_Water"); break;
                case 'O': StringCchCopy(imgfile->gardCat, 50, "Buildings,_Parts_of_Buildings,_etc."); break;
                case 'P': StringCchCopy(imgfile->gardCat, 50, "Ships_and_Parts_of_Ships"); break;
                case 'Q': StringCchCopy(imgfile->gardCat, 50, "Domestic_and_Funerary_Furniture"); break;
                case 'R': StringCchCopy(imgfile->gardCat, 50, "Temple_Furniture_and_Sacred_Emblems"); break;
                case 'S': StringCchCopy(imgfile->gardCat, 50, "Crowns,_Dress,_Staves,_etc."); break;
                case 'T': StringCchCopy(imgfile->gardCat, 50, "Warfare,_Hunting,_Butchery"); break;
                case 'U': StringCchCopy(imgfile->gardCat, 50, "Agriculture,_Crafts,_and_Professions"); break;
                case 'V': StringCchCopy(imgfile->gardCat, 50, "Rope,_Fiber,_Baskets,_Bags,_etc."); break;
                case 'W': StringCchCopy(imgfile->gardCat, 50, "Vessels_of_Stone_and_Earthenware"); break;
                case 'X': StringCchCopy(imgfile->gardCat, 50, "Loaves_and_Cakes"); break;
                case 'Y': StringCchCopy(imgfile->gardCat, 50, "Writings,_Games,_Music"); break;
                case 'Z': StringCchCopy(imgfile->gardCat, 50, "Strokes"); break;
                default: cout << "Incorrect Category Label\n"; exit (1);
            }
        }
        imgfile->gardID[i] = signNameDir[i];
        i++;
    }
    i++; // skip "-"
    int fix = i; // adjust following indexing
    while(signNameDir[i])
    {
        imgfile->signName[i - fix] = signNameDir[i];
        i++;
    }
}

void setSource(imgAttributes *imgfile, char *srcNameDir)
{
    int i = 0;
    while(srcNameDir[i])
    {
        imgfile->src[i] = srcNameDir[i];
        i++;
    }
}

void setImgPath(imgAttributes *imgfile, int maxLength, char *currentPath)
{
    StringCchCopy(imgfile->fullPath, maxLength, currentPath);
}

void setSysName(imgAttributes *imgfile, char *fileName)
{
    int i = 0;
    while(fileName[i])
    {
        imgfile->sysFileName[i] = fileName[i];
        i++;
    }
}

void setZeroAttributes(imgAttributes *imgfile)
{
    //char *begin = imgfile->gardID;
    //char *end = begin + sizeof(imgfile->gardID);
    fill(imgfile->gardID, (imgfile->gardID + sizeof(imgfile->gardID)), 0);
    fill(imgfile->gardCat, (imgfile->gardCat + sizeof(imgfile->gardCat)), 0);
    fill(imgfile->signName, (imgfile->signName + sizeof(imgfile->signName)), 0);
    fill(imgfile->src, (imgfile->src + sizeof(imgfile->src)), 0);
    fill(imgfile->sysFileName, (imgfile->sysFileName + sizeof(imgfile->sysFileName)), 0);
    fill(imgfile->fullPath, (imgfile->fullPath + sizeof(imgfile->fullPath)), 0);
}

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}
