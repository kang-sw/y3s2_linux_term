#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "types.h"
#include "uEmbedded/priority_queue.h"

enum
{
    RENDERER_NUM_BUFFER = 2
};

enum{
    STATUS_RESOURCE_ALREADY_EXIST = 1,
    ERROR_INVALID_RESOURCE_PATH = -1,
};

// Typedefs
typedef uint32_t EResourceType;

static unsigned long
hash_djb2(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

//// APIs ////
struct ProgramInstInitStruct
{
    size_t NumMaxResource;
    size_t RenderStringPoolSize;
    size_t NumMaxDrawCall;
    char const *FrameBufferDevFileName;
};

struct ProgramInstance *PInst_Create(struct ProgramInstInitStruct const *Init); 
struct ProgramInstance *PInst_Destroy(struct ProgramInstance *PInst);           // @todo

typedef uint32_t LOADRESOURCE_FLAG_T;
enum {
    LOADRESOURCE_FLAG_FONT_DEFAULT = 0,
    LOADRESOURCE_FLAG_FONT_BOLD = 1,
    LOADRESOURCE_FLAG_FONT_ITALIC = 2,
    LOADRESOURCE_IMAGE_DEFAULT = 0,
};
EStatus PInst_LoadResource(struct ProgramInstance *PInst, EResourceType Type, FHash Hash, char const *Path, LOADRESOURCE_FLAG_T Flag);
EStatus PInst_LoadFont(struct ProgramInstance* PInst, FHash Hash, char const* Path); 
struct Resource *PInst_GetResource(struct ProgramInstance *PInst, FHash Hash); // @todo
void PInst_ReleaseResource(struct ProgramInstance *PInst);                     // @todo
EStatus PInst_Update(struct ProgramInstance *PInst, float DeltaTime);          // @todo

// Draw APIs
/*! \brief              Notify ProgramInstance that queueing rendering events are done and readied to render output. Output screen will be refreshed as soon as all of the queue is processed.
    \param CamTransform Camera transform to apply.
    \return             STATUS_OK if succeed, else if failed.
 */
EStatus PInst_Flip(struct ProgramInstance *PInst, FTransform2 const *CamTransform);

EStatus PInst_RQueueText(struct ProgramInstance *PInst, FTransform2 const *Tr, struct Resource *Font, char const *String);
EStatus PInst_RQueuePolygon(struct ProgramInstance *PInst, FTransform2 const *Tr, struct Resource *Vect, uint32_t rgba);
EStatus PInst_RQueueRect(struct ProgramInstance *PInst, FTransform2 const *Tr, FVec2int v0, FVec2int v1, uint32_t rgba);
EStatus PInst_RQueueImage(struct ProgramInstance *PInst, FTransform2 const *Tr, struct Resource *Image);

// Library Dependent Code
void Internal_PInst_InitFB(struct ProgramInstance *Inst, char const *fb);
void Internal_PInst_DeinitFB(struct ProgramInstance *Inst);                           // @todo.
void *Internal_PInst_LoadImgInternal(struct ProgramInstance *Inst, char const *Path); // @todo.
void *Internal_PInst_LoadFont(struct ProgramInstance *Inst, char const *Path, LOADRESOURCE_FLAG_T FontFlag); // @todo.
void *Internal_PInst_FreeAllResource(struct Resource *rsrc);                          // @todo.

//! Program status
enum
{
    RENDERER_IDLE = 0,
    RENDERER_BUSY = 1,
    ERROR_RENDERER_INVALID = -1
};

/*! \brief Interfaces between hardware and software. */
typedef struct ProgramInstance
{
    LPTYPEID id;

    // Frame buffer handle. Also used to trigger thread shutdown.
    void *hFB;

    // Resource management
    struct Resource *arrResource;
    size_t NumResource;
    size_t NumMaxResource;

    // Double buffered draw arg pool
    int ActiveBuffer; // 0 or 1.

    // Rendering event memory pool. Double buffered.
    char *RenderStringPool[RENDERER_NUM_BUFFER];
    size_t StringPoolHeadIndex[RENDERER_NUM_BUFFER];
    size_t StringPoolMaxSize;

    // Evenr argument memory pool
    struct RenderEventArg *arrRenderEventArgPool[RENDERER_NUM_BUFFER];
    size_t PoolHeadIndex[RENDERER_NUM_BUFFER];
    size_t PoolMaxSize;

    // Priority queue for manage event objects
    pqueue_t RenderEventQueue[RENDERER_NUM_BUFFER];
    
    // Thread handle of rendering thread
    pthread_t ThreadHandle;
} UProgramInstance;

enum
{
    RESOURCE_LINEVECTOR,
    RESOURCE_IMAGE,
    RESOURCE_FONT
};

typedef struct Resource
{
    /* data */
    uint32_t Hash;
    EResourceType Type;
    void *data;
} UResource;

/*! \brief Type of rendering event. */
typedef enum
{
    ERET_NONE = 0, // Nothing
    ERET_TEXT,     // Text
    ERET_POLY,     // Empty Polygon
    ERET_RECT,     // Filled Rectangle
    ERET_IMAGE
} ERenderEventType;

/*! \brief Text rendering event data structure */
struct RenderEventData_Text
{
    // Length of string
    size_t StrLen;
    // Name of this argument will indicate the string address.
    char str[4];
};

struct RenderEventData_Polylines
{
    struct Resource *PolyLines;
    uint8_t rgba[4];
};

struct RenderEventData_Rectangle
{
    int32_t x0, y0;
    int32_t x1, y1;
    uint8_t rgba[4];
};

struct RenderEventData_IMAGE
{
    struct Resource *Image;
};

typedef union {
    struct RenderEventData_Text Text;
    struct RenderEventData_Polylines Poly;
    struct RenderEventData_Rectangle Rect;
    struct RenderEventData_IMAGE Image;
} FRenderEventData;

typedef struct RenderEventArg
{
    int32_t Layer;
    ERenderEventType Type;
    FTransform2 Transform;
    FRenderEventData Data;
} FRenderEventArg;
