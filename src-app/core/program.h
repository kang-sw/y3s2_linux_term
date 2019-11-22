#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "types.h"
#include "uEmbedded/priority_queue.h"

//! \brief Miscellaneous constant values
enum
{
    RENDERER_NUM_BUFFER = 2,
    STATUS_RESOURCE_ALREADY_EXIST = 1,
    ERROR_INVALID_RESOURCE_PATH = -1,
};

//! \brief Resource Type indicator.
typedef uint32_t EResourceType;

//! \brief Primarily used hash function for all string indicators.
static unsigned long
hash_djb2(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*! \brief Program instance initialize information descriptor. */
struct ProgramInstInitStruct
{
    //! Maximum loadable resource count value.
    size_t NumMaxResource;
    //! Buffer size of string pool on render.
    size_t RenderStringPoolSize;
    //! Maximum draw call count per rendering.
    size_t NumMaxDrawCall;
    //! \brief Frame buffer's device file name.
    //! \details If Set NULL, fb0 will automatically be selected.
    char const *FrameBufferDevFileName;
};

/*! \brief Create new program instance.
    \param Init Initalizer struct.
    \return Reference for newly created program instance.
 */
struct ProgramInstance *PInst_Create(struct ProgramInstInitStruct const *Init);

/*! \brief Destroy given program instance reference. */
void PInst_Destroy(struct ProgramInstance *PInst);

//! Resource flag type.
typedef uint32_t LOADRESOURCE_FLAG_T;

//! Load resource flag values.
enum
{
    LOADRESOURCE_FLAG_FONT_DEFAULT = 0,
    LOADRESOURCE_FLAG_FONT_BOLD = 1,
    LOADRESOURCE_FLAG_FONT_ITALIC = 2,
    LOADRESOURCE_IMAGE_DEFAULT = 0,
};

/*! \brief Load given resource type to given program instance.
    \param PInst Instance to load resource
    \param Type Type of resource to load.
    \param Hash Hash value generated by specific algorithm.
    \param Path Path to given resource.
    \param Flag Flag that indicates specific load options.
    \return Resource loading result.
 */
EStatus PInst_LoadResource(struct ProgramInstance *PInst, EResourceType Type, FHash Hash, char const *Path, LOADRESOURCE_FLAG_T Flag);

/*! \brief Find resource by Hash. Returns NULL if no resource exists for given hash.
    \param PInst 
    \param Hash 
    \return 
 */
struct Resource *PInst_GetResource(struct ProgramInstance *PInst, FHash Hash);

//! Will not be implemented for this project.
void PInst_ReleaseResource(struct ProgramInstance *PInst); // @todo

/*! \brief Update program instance
    \param PInst 
    \param DeltaTime Delta time in seconds.
    \return Current system status. Returns non-zero value for warnings/errors.
 */
EStatus PInst_Update(struct ProgramInstance *PInst, float DeltaTime); // @todo

// Draw APIs
/*! \brief   Request draw.           
    \param CamTransform Camera transform to apply.
    \return STATUS_OK if succeed, else if failed.
    \details 
        Notify ProgramInstance that queueing rendering events are done and readied to render output. Output screen will be refreshed as soon as all of the queue is processed.
 */
EStatus PInst_Flip(struct ProgramInstance *PInst, FTransform2 const *CamTransform);

//! Color descriptor for draw call
typedef struct Color
{
    float A;
    float R;
    float G;
    float B;
} FColor;

typedef struct Color const *COLORREF;

/*! \brief Queue string rendering
    \param PInst 
    \param Tr Transform
    \param Font Font resource data
    \param String String to output. Will be copied.
    \param 
    \return Request result.
 */
EStatus PInst_RQueueText(
    struct ProgramInstance *PInst,
    FTransform2 const *Tr,
    struct Resource *Font,
    char const *String,
    COLORREF rgba);

//! Will not be implemented.
EStatus PInst_RQueuePolygon(struct ProgramInstance *PInst, FTransform2 const *Tr, struct Resource *Vect, COLORREF rgba);

/*! \brief Queue filled rectangle rendering
    \param Tr   Transform of ractangle.
    \param pos  Base position of ractangle.
    \param size Size of ractangle
    \return 
 */
EStatus PInst_RQueueRect(struct ProgramInstance *PInst, FTransform2 const *Tr, FVec2int ofst, FVec2int size, COLORREF rgba);

/*! \brief Queue image draw call.
    \param PInst 
    \param Tr 
    \param Image 
    \return 
 */
EStatus PInst_RQueueImage(struct ProgramInstance *PInst, FTransform2 const *Tr, struct Resource *Image);

// For library implementations
void Internal_PInst_InitFB(struct ProgramInstance *Inst, char const *fb);
void Internal_PInst_DeinitFB(struct ProgramInstance *Inst); // @todo.
void *Internal_PInst_LoadImgInternal(struct ProgramInstance *Inst, char const *Path);
void *Internal_PInst_LoadFont(struct ProgramInstance *Inst, char const *Path, LOADRESOURCE_FLAG_T FontFlag);
void *Internal_PInst_FreeAllResource(struct Resource *rsrc); // @todo.
// Forward declaration to render;
struct RenderEventArg;
void Internal_PInst_Draw(void *hFB, struct RenderEventArg const *Arg); // @todo.
void Internal_PInst_Flush(void *hFB);                                  // @todo.

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
    pqueue_t arrRenderEventQueue[RENDERER_NUM_BUFFER];

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
    uint8_t rgba[4];
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
