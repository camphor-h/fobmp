#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <SDL2/SDL.h>
#define FILE_HEAD_LENGTH 14
#define VERSION_LENGTH 5
#define TRUE 1
#define FALSE 0
typedef char BOOL;
const char BMP_FILE_HEAD[2] = {'B', 'M'};
const char VERSION[VERSION_LENGTH] = "0.25";
typedef struct Row
{
    uint8_t* bytes;
    struct Row* next;
}Row;
typedef struct ColorPalette
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
}ColorPalette;
typedef struct InfoHeader
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t hPelsPerMeter;
    int32_t vPelsPerMeter;
    uint32_t color;
    uint32_t importantColor;
}InfoHeader;
typedef struct BMPFileHeader
{
    char signature[2];
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t dataOffSet;
}BMPFileHeader;
typedef struct BMPfile
{
    FILE* file;
    BMPFileHeader bmpfileheader;
    uint32_t infoHeaderSize;
    void* infoHeader;
    ColorPalette* colorPalette;
    uint32_t rawRowSize;
    uint32_t stride;
    uint32_t paddingBytes;
    Row* rowHead;
}BMPfile;
int readFile(BMPfile*);
void readFileHeader(BMPfile*);
void readInfoHeader(BMPfile*);
void readColorPalettes(BMPfile*);
void readBitmap(BMPfile*);
void getRawRowSize(BMPfile*);
void getStride(BMPfile*);
void getPaddingBytes(BMPfile*);
void error(int);
void printAllInfo(BMPfile*);
void freeAll(BMPfile*);
void createRows(BMPfile*);
void readRows(BMPfile*);
void display(BMPfile*);
inline int32_t abs(int32_t number) {return (number > 0 ? number : -number);}
void createBMPTexture(BMPfile*, SDL_Texture*);
int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        return 0;
    }
    else
    {
        FILE* file = fopen(argv[1], "rb");
        if (file == NULL)
        {
            error(1);
        }
        else
        {
            BMPfile bmpfile;
            bmpfile.colorPalette = NULL;
            bmpfile.file = file;
            readFile(&bmpfile);
            BOOL showInfo = TRUE;
            BOOL showDisplay = TRUE;
            for (int i = 2; i < argc; i++)
            {
                showInfo = (strcmp(argv[i], "-noinfo") == 0) ? FALSE : TRUE;
                showDisplay = (strcmp(argv[i], "-nodisplay") == 0) ? FALSE : TRUE;
            }
            if (showInfo == TRUE)
            {
                printAllInfo(&bmpfile);
            }
            if (showDisplay == TRUE)
            {
                display(&bmpfile);
            }
            freeAll(&bmpfile);
        }
    }
}
int readFile(BMPfile* bmpfile)
{
    readFileHeader(bmpfile);
    if (bmpfile->bmpfileheader.signature[0] != BMP_FILE_HEAD[0] || bmpfile->bmpfileheader.signature[1] != BMP_FILE_HEAD[1])
    {
        error(2);
    }
    long current_pos = ftell(bmpfile->file);
    fread(&(bmpfile->infoHeaderSize), 4, 1, bmpfile->file);
    if (bmpfile->infoHeaderSize == 40)
    {
        bmpfile->infoHeader = malloc(bmpfile->infoHeaderSize);
        fseek(bmpfile->file, current_pos, SEEK_SET);
        readInfoHeader(bmpfile);
    }
    else
    {
        error(3);
    }
    if (((InfoHeader*)(bmpfile->infoHeader))->bitCount == 1 || ((InfoHeader*)(bmpfile->infoHeader))->bitCount == 4 || ((InfoHeader*)(bmpfile->infoHeader))->bitCount == 8)
    {
        fseek(bmpfile->file, FILE_HEAD_LENGTH + bmpfile->infoHeaderSize, SEEK_SET);
        readColorPalettes(bmpfile);
    }
    readBitmap(bmpfile);
}
void readFileHeader(BMPfile* bmpfile)
{
    fread(&(bmpfile->bmpfileheader.signature), 2, 1, bmpfile->file);
    fread(&(bmpfile->bmpfileheader.fileSize), 4, 1, bmpfile->file);
    fread(&(bmpfile->bmpfileheader.reserved1), 2, 1, bmpfile->file);
    fread(&(bmpfile->bmpfileheader.reserved2), 2, 1, bmpfile->file);
    fread(&(bmpfile->bmpfileheader.dataOffSet), 4, 1, bmpfile->file);
}
void readInfoHeader(BMPfile* bmpfile)
{
    //fread((InfoHeader*)bmpfile->infoHeader, 40, 1, bmpfile->file);
    
    fread(&(((InfoHeader*)bmpfile->infoHeader)->size), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->width), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->height), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->planes), 2, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->bitCount), 2, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->compression), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->imageSize), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->hPelsPerMeter), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->vPelsPerMeter), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->color), 4, 1, bmpfile->file);
    fread(&(((InfoHeader*)bmpfile->infoHeader)->importantColor), 4, 1, bmpfile->file);
    
}
void readColorPalettes(BMPfile* bmpfile)
{
    int usedColor;
    if (((InfoHeader*)(bmpfile->infoHeader))->color == 0)
    {
        usedColor = 1 << ((InfoHeader*)(bmpfile->infoHeader))->bitCount;
    }
    else
    {
        usedColor = ((InfoHeader*)(bmpfile->infoHeader))->color;
    }
    bmpfile->colorPalette = malloc(usedColor * sizeof(ColorPalette));
    ColorPalette* current = bmpfile->colorPalette;
    for (int i = 0; i < usedColor; i++)
    {
        fread(current, sizeof(ColorPalette), 1, bmpfile->file);
        current++;
    }
}
void readBitmap(BMPfile* bmpfile)
{
    if ((((InfoHeader*)(bmpfile->infoHeader))->bitCount == 24 || ((InfoHeader*)(bmpfile->infoHeader))->bitCount == 32))
    {
        getRawRowSize(bmpfile);
        getStride(bmpfile);
        getPaddingBytes(bmpfile);
    }
    createRows(bmpfile);
    readRows(bmpfile);
}
void getRawRowSize(BMPfile* bmpfile)
{
    bmpfile->rawRowSize = ((((((InfoHeader*)(bmpfile->infoHeader))->width) * (((InfoHeader*)(bmpfile->infoHeader))->bitCount)) + 7) / 8);
}
void getStride(BMPfile* bmpfile)
{
    bmpfile->stride = ((bmpfile->rawRowSize + 3) & ~3);
}
void getPaddingBytes(BMPfile* bmpfile)
{
    bmpfile->paddingBytes = bmpfile->stride - bmpfile->rawRowSize;
}
void printAllInfo(BMPfile* bmpfile)
{
    printf("File Size: %" PRIu32 " Bytes\n", bmpfile->bmpfileheader.fileSize);
    printf("Info header Size: %" PRIu32 " Bytes\n", bmpfile->infoHeaderSize);
    printf("Width: %" PRId32 " Pixels\n", ((InfoHeader*)bmpfile->infoHeader)->width);
    printf("Height: %" PRId32 " Pixels\n", ((InfoHeader*)bmpfile->infoHeader)->height);
    printf("Horizontal Resolution: %" PRId32 " Pixels per Meter\n", ((InfoHeader*)bmpfile->infoHeader)->hPelsPerMeter);
    printf("Vertical Resolution: %" PRId32 " Pixels per Meter\n", ((InfoHeader*)bmpfile->infoHeader)->vPelsPerMeter);
    printf("Bit Depth (Bit Count): %" PRIu16 " Bits\n", ((InfoHeader*)bmpfile->infoHeader)->bitCount);
    printf("Compression Method: %" PRIu32 , ((InfoHeader*)bmpfile->infoHeader)->compression);
    if (((InfoHeader*)bmpfile->infoHeader)->compression == 0)
    {
        printf(" (No Compression)\n");
    }
    printf("------------------------\n");
    printf("fobmp version: %s\n", VERSION);
}
void error(int value)
{
    printf("Error:");
    switch (value)
    {
        case 1:
            printf("Cannot Find File.");
            break;
        case 2:
            printf("Cannot open file. It doesn't have a proper BMP file header.");
            break;
        case 3:
            printf("Error:Unsupported info header size.");
            break;
        case 4:
            printf("Failed to allocate memory.");
            break;
        case 5:
            printf("Failed to read BMP file data.");
            break;
        case 6:
            printf("Unsupported Bit Count(Bit Depth).");
            break;
        case 7:
            printf("Cannot init SDL.");
            break;
        default:
            printf("Unknown Error.");
    }
    printf("\n");
    exit(value);
}
void createRows(BMPfile* bmpfile)
{
    Row* curRow = NULL;
    Row* preRow = NULL;
    int32_t absHeight = abs(((InfoHeader*)(bmpfile->infoHeader))->height);
    for (int i = 0; i < absHeight; i++)
    {
        curRow = malloc(sizeof(Row));
        if (curRow == NULL)
        {
            error(4);
        }
        curRow->next = NULL;
        if (preRow == NULL)
        {
            bmpfile->rowHead = curRow;
        }
        else
        {
            preRow->next = curRow;
        }
        preRow = curRow;
    }
}
void readRows(BMPfile* bmpfile)
{
    fseek(bmpfile->file, bmpfile->bmpfileheader.dataOffSet, SEEK_SET);
    Row* handleRow = bmpfile->rowHead;
    while (handleRow != NULL)
    {
        handleRow->bytes = malloc(bmpfile->stride);
        if (handleRow->bytes == NULL)
        {
            error(4);
        }
        if (fread(handleRow->bytes, bmpfile->stride, 1, bmpfile->file) != 1)
        {
            error(5);
        }
        handleRow = handleRow->next;
    }
}
void freeAll(BMPfile* bmpfile)
{
    //Things about SDL will be destroy in display()
    free(bmpfile->infoHeader);
    fclose(bmpfile->file);
    if (((InfoHeader*)(bmpfile->infoHeader))->bitCount == 1 || ((InfoHeader*)(bmpfile->infoHeader))->bitCount == 4 || ((InfoHeader*)(bmpfile->infoHeader))->bitCount == 8)
    {
        free(bmpfile->colorPalette);
    }
    Row* current = bmpfile->rowHead;
    while (current != NULL)
    {
        Row* next = current->next;
        free(current->bytes);
        free(current);
        current = next;
    }
}





void display(BMPfile* bmpfile)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        error(7);
    }
    SDL_Window* window = SDL_CreateWindow("Fobmp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,  ((InfoHeader*)bmpfile->infoHeader)->width, ((InfoHeader*)bmpfile->infoHeader)->height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_PixelFormatEnum pixelFormat;
    if (((InfoHeader*)bmpfile->infoHeader)->bitCount == 24)
    {
        pixelFormat = SDL_PIXELFORMAT_BGR24;
    }
    else
    {
        error(6);
    }
    SDL_Texture* pictureTexture = SDL_CreateTexture(renderer, pixelFormat, SDL_TEXTUREACCESS_STREAMING, ((InfoHeader*)bmpfile->infoHeader)->width, ((InfoHeader*)bmpfile->infoHeader)->height);
    createBMPTexture(bmpfile, pictureTexture);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, pictureTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    SDL_Event event;
    BOOL quit = FALSE;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = TRUE;
            }
        }
    }
    SDL_DestroyTexture(pictureTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
void createBMPTexture(BMPfile* bmpfile, SDL_Texture* texture)
{
    void* pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    uint32_t* pixelData = (uint32_t*)pixels;
    Row* curRow = bmpfile->rowHead;
    int height = abs(((InfoHeader*)(bmpfile->infoHeader))->height);
    Row** rows = (Row**)malloc(height * sizeof(Row*));
    Row* current = bmpfile->rowHead;
    for (int i = 0; i < height && current != NULL; i++)
    {
        rows[i] = current;
        current = current->next;
    }
    for (int y = 0; y < height; y++)
    {
        int src_y = height - 1 - y;
        if (rows[src_y] && rows[src_y]->bytes) {
            uint8_t* dest = (uint8_t*)pixels + y * pitch;
            memcpy(dest, rows[src_y]->bytes, bmpfile->stride);
        }
    }
    free(rows);
    SDL_UnlockTexture(texture);
}