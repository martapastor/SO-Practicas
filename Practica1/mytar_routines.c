#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes, uint16_t *crc)
{
	int totalBytes = 0; // Amount of bytes copied to file
    int destinationByte = 0; // Byte (only one each) written to destination file
    int originByte = 0;  // Byte (only one each) read from the origin file

    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

	// If the origin file does not exist, no bytes are copied to the destination
	// file and therefore, we return a -1 value to indicate an error has occured
	if (origin == NULL) {
		return -1;
	}

	// While the number of total bytes copied until the moment is lower than the
	// total number of bytes we have to copy, we continue in the loop. Moreover,
	// while the byte read from the origin file is not an EOF flag, we enter the
	// loop:
    while ((totalBytes < nBytes) && (originByte = getc(origin)) != EOF) {
		// We add to the destination file the char read from the origin file and
		// which we have saved in the originByte variable:
        sum1 = (sum1 + (uint8_t)originByte) % 255;
        sum2 = (sum2 + sum1) % 255;

        destinationByte = putc(originByte, destination);

        totalBytes++;
    }

	(*crc) = (sum2 << 8) | sum1;

    return totalBytes;
}

/** Loads a string from a .mtar file.
 *
 * file: pointer to the FILE descriptor
 *
 * The loadstr() function must allocate memory from the heap to store
 * the contents of the string read from the FILE.
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc())
 *
 * Returns: !=NULL if success, NULL if error
 */
char*
loadstr(FILE * file)
{
	int filenameLength = 0;
    char *filename = NULL;
    char c;

	// While the char we are reading from the file passed as argument is not the
	// special char \0, we increment in 1 the filenameLength counter and
	// continue reading the chars:
    while((c = getc(file) != '\0')) {
        filenameLength++;
    }

    if (filenameLength == 0) {
        return NULL;
    }

	// We allocate dynamic memory for the filename plus 1 extra space for the \0
	// special char:
    filename = malloc(sizeof(char) * (filenameLength + 1));

    // We shift the PC of file from the current position to filenameLength + 1
    // positions to the left (that is why the value is negative):
    fseek(file, -(filenameLength + 1), SEEK_CUR);
    // We could also use the fseek function with the SEEK_SET position and an
    // offset equal to 0:
    // fseek(file, 0, SEEK_SET);

    // For each of the chars in the filename (including the \0 special one), we
    // store in the apropiate position the value of the char read from the file:
    for (int i = 0; i < filenameLength + 1; i++) {
        filename[i] = getc(file);
    }

    // Once the string has been properly built in memory, the function returns
    // the starting address of the string
    return filename;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor
 * nFiles: output parameter. Used to return the number
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry*
readHeader(FILE * tarFile, int * nFiles)
{
    int numOfCompressedFiles = 0, fileSize = 0;
    uint16_t crc = 0x1234;
    stHeaderEntry *stHeader = NULL;

    // We check the number of files compressed in the .mtar by reading 1 item
    // (third argument) of size correponding to an int (second argument), which
    // integer will be the first value of the header that indicates the number
    // of files in the archive:
    if (fread(&numOfCompressedFiles, sizeof(int), 1, tarFile) == 0) {
        // If the function returns 0 that means no items have been correctly read
        // and an error is return:
        return NULL;
    }

    // We allocate dynamic memory for as many headers as the number of compressed
    // files in the archive:
    stHeader = malloc(sizeof(stHeaderEntry) * numOfCompressedFiles);

    // For each of the files compressed in the archive, we will try to read their
    // filenames:
    for (int i = 0; i < numOfCompressedFiles; i++) {
        // If the loadstr function cannot read the name of the file, it returns
        // an error:
        if ((stHeader[i].name = loadstr(tarFile)) == NULL) {
            return NULL;
        }

        // Otherwise, it reads the size of the compressed file and saves it to
        // the apropiate position of the struct header array, next to the name:
        fread(&fileSize, sizeof(unsigned int), 1, tarFile);
        stHeader[i].size = fileSize;
        // And then reads the value of the CRC for the current file and also
        // stores it in the proper field of the struct:
        fread(&crc, sizeof(uint16_t), 1, tarFile);
        stHeader[i].crc = crc;
    }

    // We return the number of files compressed for the .mtar archive, and the
    // correctly built header:
    (*nFiles) = numOfCompressedFiles;

    return stHeader;
}

/** Creates a tarball archive
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE.
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive.
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size)
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int
createTar(int nFiles, char *fileNames[], char tarName[]) {
    FILE * originFile; // Used for reading each .txt file
    FILE * destinationFile; // Used for writing in the output file.
    uint16_t crc = 0x1234;

    int copiedBytes = 0, stHeaderBytes = 0;
    stHeaderEntry *stHeader;

    /* ---------- HEADER SIZE SECTION ---------- */

    // We create a stHeader in the heap with the proper size.
    // First, we allocate memory for an array of stHeader structs:
    stHeader = malloc(sizeof(stHeaderEntry) * nFiles);
    // Second, we store in the stHeaderBytes the size (in bytes) of the header,
    // that corresponds to the first integer which determines the number of
    // compressed files in the .mtar archive, and for each of that compressed
    // files, we also store it size taking into account that are unsigned ints:
    stHeaderBytes = sizeof(int) + sizeof(unsigned int) * nFiles + sizeof(uint16_t) * nFiles;

    // For each of the compressed files in the .mtar archive:
    for (int i = 0; i < nFiles; i++) {
        // We add to the header size counter the bytes for each filename (+1 for
        // the \0 special char):
        stHeaderBytes += strlen(fileNames[i]) + 1;
    }

    // Now, we open the tarName.mtar file for writing the header and data of the
    // compressed files in it...
    destinationFile =  fopen(tarName, "w");
    //... and move the pointer of said output file to the content-of-the-files
    // section, leaving space at the beginning of the output file to store the
    // information realted to the header, which we will store at the end of the
    // this function. More specifically, we shift the PC pointer an offset to the
    // right corresponding to the size of the header information, starting from
    // the beginning of the file (that is what SEEK_SET represents):
    fseek(destinationFile, stHeaderBytes, SEEK_SET);

    /* ---------- DATA STORAGE SECTION ---------- */

    // Now, for each of the compressed files in the .mtar archive:
    for (int i = 0; i < nFiles; i++) {
        // We try to open the file whose name is stored in the fileNames array:
        if ((originFile = fopen(fileNames[i], "r")) == NULL) {
            return EXIT_FAILURE;
        }

        // We copy a very large number of bytes to the output file, to ensure
        // that we copy all the chars:
        copiedBytes = copynFile(originFile, destinationFile, INT_MAX, &crc);

        // If no char has been copied, an error has occured:
        if (copiedBytes == -1) {
            return EXIT_FAILURE;
        }
        else {
            stHeader[i].name = malloc(sizeof(fileNames[i]) + 1);
            stHeader[i].size = copiedBytes;
            stHeader[i].crc = crc;

            // We copy the filename string to the struct entry:
            strcpy(stHeader[i].name, fileNames[i]);

            printf("[%d]: Adding file %s, size %d Bytes, CRC 0x%.4X\n", i, stHeader[i].name, stHeader[i].size, stHeader[i].crc);
        }

        // If we cannot close the file from where we are reading the info, an error
        // has occured:
        if (fclose(originFile) == EOF) {
            return EXIT_FAILURE;
        }
    }

    /*  ---------- HEADER STORAGE SECTION ---------- */

    // Now, we move the PC pointer to the beginning of the file in order to write
    // the total number of compressed files in the .mtar archive and the header
    // information of each one:
    if (fseek(destinationFile, 0, SEEK_SET) != 0) {
        // If the PC has not been properly moved to the beginning of the file,
        // an error has occured:
        return EXIT_FAILURE;
    }
    else {
        fwrite(&nFiles, sizeof(int), 1, destinationFile);
    }

    // For each compressed files in the .mtar archive, we write their headers:
    for (int i = 0; i < nFiles; i++) {
        fwrite(stHeader[i].name, strlen(stHeader[i].name) + 1, 1, destinationFile);
        fwrite(&stHeader[i].size, sizeof(unsigned int), 1, destinationFile);
        fwrite(&stHeader[i].crc, sizeof(uint16_t), 1, destinationFile);
    }

    // Finally, we have to free memory allocated through all the function, and
    // erase all structs that have been created:
    for (int i = 0; i < nFiles; i++) {
        free(stHeader[i].name);
    }

    free(stHeader);

    // If we cannot close the file to where we are writting the info, an error
    // has occured:
    if (fclose(destinationFile) == EOF) {
        return EXIT_FAILURE;
    }

    printf("Your .mtar file has been successfully created!\n");

    return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE.
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the
 * tarball's data section. By using information from the
 * header --number of files and (file name, file size) pairs--, extract files
 * stored in the data section of the tarball.
 *
 */
int
extractTar(char tarName[]) {
    FILE *tarFile = NULL;
    FILE *destinationFile = NULL;
    stHeaderEntry *stHeader;

    int numOfCompressedFiles = 0, copiedBytes = 0;

    // We try to open the file whose name is stored in the tarName array. If we
    // cannot open it, an error has occured:
    if((tarFile = fopen(tarName, "r") ) == NULL) {
        return EXIT_FAILURE;
    }

    // We return in the stHeader the array of struct for each file, and in
    // numOfCompressedFiles the total number of compressed files in the .mtar:
    if ((stHeader = readHeader(tarFile, &numOfCompressedFiles)) == NULL) {
        return EXIT_FAILURE;
    }

    // We write the content of each compressed file to the destination files:
    for (int i = 0; i < numOfCompressedFiles; i++) {
        if ((destinationFile = fopen(stHeader[i].name, "w")) == NULL) {
            return EXIT_FAILURE;
        }
        else {
            copiedBytes = copynFile(tarFile, destinationFile, stHeader[i].size, &stHeader[i].crc);

            if (copiedBytes == -1) {
                return EXIT_FAILURE;
            }

            printf("[%d]: Creating file %s, size %d Bytes, CRC 0x%.4X ... \n", i, stHeader[i].name, stHeader[i].size, stHeader[i].crc);
            printf("[%d]: CRC of extracted file 0x%.4X. File is OK.\n", i, stHeader[i].crc);
        }

        if(fclose(destinationFile) != 0) {
            return EXIT_FAILURE;
        }
    }

    // Finally, we have to free memory allocated through all the function, and
    // erase all structs that have been created:
    for (int i = 0; i < numOfCompressedFiles; i++) {
        free(stHeader[i].name);
    }

    free(stHeader);

    if (fclose(tarFile) == EOF) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
