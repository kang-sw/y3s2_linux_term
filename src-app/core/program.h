#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "types.h"
#include "uEmbedded/priority_queue.h"
#include "uEmbedded/timer_logic.h"

//! \brief Miscellaneous constant values
enum
{
    RENDERER_NUM_MAX_BUFFER = 2,
    STATUS_RESOURCE_ALREADY_EXIST = 1,
    ERROR_INVALID_RESOURCE_PATH = -1,
    ERROR_DRAW_CALL_OVERFLOW = -2
};

//! \brief Resource Type indicator.
typedef uint32_t EResourceType;
typedef struct ProgramInstance UProgramInstance;

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
    //! Number of maximum timer nodes
    size_t NumMaxTimer;
};

static void PInst_InitializeInitStruct(struct ProgramInstInitStruct *v)
{
    v->NumMaxDrawCall = 0x2000;
    v->RenderStringPoolSize = 0x2000;
    v->NumMaxResource = 0x1000;
    v->FrameBufferDevFileName = NULL;
    v->NumMaxTimer = 0x1000;
}

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

/*! \brief Queue timer in milliseconds
    \param Callback 
    \param CallbackArg 
    \param delay_ms Delay time in milliseconds
    \return Handle of assigned timer.
 */
timer_handle_t PInst_QueueTimer(struct ProgramInstance *PInst, void (*Callback)(void *), void *CallbackArg, size_t delay_ms);

/*! \brief Abort timer 
    \param PInst 
    \param handle 
    \return True if given timer handle was valid, and canceled successfully.
 */
bool PInst_AbortTimer(struct ProgramInstance *PInst, timer_handle_t handle);

/*! \brief Get Timer Delay Left.
    \param PInst 
    \param handle Timer handle.
    \return Time left in milliseconds.
 */
size_t PInst_GetTimerDelayLeft(struct ProgramInstance *PInst, timer_handle_t handle);

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
EStatus PInst_UpdateTimer(struct ProgramInstance *PInst, float DeltaTime);

// Draw APIs
/*! \brief   Request draw.           
    \param CamTransform Camera transform to apply.
    \return STATUS_OK if succeed, else if failed.
    \details 
        Notify ProgramInstance that queueing rendering events are done and readied to render output. Output screen will be refreshed as soon as all of the queue is processed.
 */
EStatus PInst_Flip(struct ProgramInstance *PInst);

/*! \brief Set camera tranform for next frame. */
void PInst_SetCameraTransform(struct ProgramInstance *s, FTransform2 const *v);

//! Returns handle of aspect ratio.
float *PInst_AspectRatio(struct ProgramInstance *s);

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
    \param Layer Objects with high layer values are drawn in front.
    \param Tr Transform
    \param Font Font resource data
    \param String String to output. Will be copied.
    \param 
    \return Request result.
 */
EStatus PInst_RQueueText(
    struct ProgramInstance *PInst,
    int32_t Layer,
    FTransform2 const *Tr,
    struct Resource *Font,
    char const *String,
    COLORREF rgba);

//! Will not be implemented.
EStatus PInst_RQueuePolygon(struct ProgramInstance *PInst, int32_t Layer, FTransform2 const *Tr, struct Resource *Vect, COLORREF rgba);

/*! \brief Queue filled rectangle rendering
    \param Tr   Transform of ractangle.
    \param Layer Objects with high layer values are drawn in front.
    \param pos  Base position of ractangle.
    \param size Size of ractangle
    \return 
 */
EStatus PInst_RQueueRect(struct ProgramInstance *PInst, int32_t Layer, FTransform2 const *Tr, FVec2int ofst, FVec2int size, COLORREF rgba);

/*! \brief Queue image draw call.
    \param PInst 
    \param Tr 
    \param Image 
    \return 
 */
EStatus PInst_RQueueImage(struct ProgramInstance *PInst, int32_t Layer, FTransform2 const *Tr, struct Resource *Image);

// For library implementations
void *Internal_PInst_InitFB(struct ProgramInstance *Inst, char const *fb);
void Internal_PInst_DeinitFB(struct ProgramInstance *Inst, void *hFB); // @todo.
void *Internal_PInst_LoadImgInternal(struct ProgramInstance *Inst, char const *Path);
void *Internal_PInst_LoadFont(struct ProgramInstance *Inst, char const *Path, LOADRESOURCE_FLAG_T FontFlag);
void *Internal_PInst_FreeAllResource(struct Resource *rsrc); // @todo.
// Forward declaration to render;
struct RenderEventArg;
void Internal_PInst_Predraw(void *hFB, int ActiveBuffer);                                // @todo.
void Internal_PInst_Draw(void *hFB, struct RenderEventArg const *Arg, int ActiveBuffer); // @todo.
void Internal_PInst_Flush(void *hFB, int ActiveBuffer);                                  // @todo.

//! Program status
enum
{
    RENDERER_IDLE = 0,
    RENDERER_BUSY = 1,
    ERROR_RENDERER_INVALID = -1
};

enum
{
    RESOURCE_NONE,
    RESOURCE_LINEVECTOR,
    RESOURCE_IMAGE,
    RESOURCE_FONT
};

typedef struct Resource UResource;
