/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_hi_wrap.c
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : Virtual wrap for Hi3516 sdk
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "dem_globals.h"
#include "dem_config.h"
#include "dem_hi_wrap.h"

#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "mpi_vi.h"
#include "mpi_sys.h"
#include "mpi_region.h"
#include "mpi_ive.h"

#define MIN_BMP_RGN_HANDLEID            (40)

typedef struct RGN_PRAMA_HANDLE
{
    int max_handle;             // max rgn handle id is used
}RGN_PRAMA_HANDLE;

static RGN_PRAMA_HANDLE s_rgn_params;

/*******************************************************************************
 * name    : HI_draw_cover_boxes
 * function: draw the boxes.
 * input   : tuple_num: coordinates tuple number
 *           cords:  boxes coordinates, such as X1, Y1, X2, Y2
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_draw_cover_boxes(int tuple_num, int *cords)
{
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]DEM_cfg_get_global_params fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }
    
    int i = 0;
    int hi_ret = DEM_COMMONFAIL;
    MPP_CHN_S mpp_chn;
    RGN_ATTR_S rgn_attr;
    RGN_CHN_ATTR_S rgn_chn_attr;

    /* Add cover to vpss group */
    mpp_chn.enModId  = HI_ID_VPSS;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = cfg->osd.attach_vpss_id;

    /* detach old rgn */
    for (i = 0; i < s_rgn_params.max_handle; i++)
    {
        hi_ret = HI_MPI_RGN_DetachFromChn(i, &mpp_chn);
        if (HI_SUCCESS != hi_ret)
        {
            printf("[%s: %d]HI_MPI_RGN_DetachFromChn fail! s32Ret: 0x%x.\n", __func__, __LINE__, hi_ret);
            return hi_ret;
        }
    }

    /* Create cover and attach to vpss group */
    int *coords = cords;
    for (i = 0; i < tuple_num; i++)
    {
        // read the address of coordinates
        coords = cords + 4 * i;
        int x1 = *coords;
        int y1 = *(coords + 1);
        int x2 = *(coords + 2);
        int y2 = *(coords + 3);

        // set the rgn attributes
        rgn_attr.enType = COVEREX_RGN;

        // we cann't create a same handle twice
        if (s_rgn_params.max_handle <= i)
        {
            hi_ret = HI_MPI_RGN_Create(i, &rgn_attr);
            if (HI_SUCCESS != hi_ret)
            {
                printf("[%s: %d]HI_MPI_RGN_Create fail! s32Ret: 0x%x.\n", __func__, __LINE__, hi_ret);
                continue;
            }
        }

        rgn_chn_attr.bShow  = HI_TRUE;
        rgn_chn_attr.enType = COVEREX_RGN;

        rgn_chn_attr.unChnAttr.stCoverExChn.enCoverType = AREA_QUAD_RANGLE;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.bSolid = HI_FALSE;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.u32Thick = 4;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[0].s32X = x1;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[0].s32Y = y1;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[1].s32X = x2;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[1].s32Y = y1;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[2].s32X = x1;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[2].s32Y = y2;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[3].s32X = x2;
        rgn_chn_attr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[3].s32Y = y2;

        rgn_chn_attr.unChnAttr.stCoverExChn.u32Color         = 0x000000ff;
        if (1 == i % COVEREX_MAX_NUM_VPSS)
        {
            rgn_chn_attr.unChnAttr.stCoverExChn.u32Color     = 0x0000ff00;
        }
        else if (2 == i % COVEREX_MAX_NUM_VPSS)
        {
            rgn_chn_attr.unChnAttr.stCoverExChn.u32Color     = 0x00ff0000;
        }
        else if (3 == i % COVEREX_MAX_NUM_VPSS)
        {
            rgn_chn_attr.unChnAttr.stCoverExChn.u32Color     = 0x00808080;
        }
        else
        {
            rgn_chn_attr.unChnAttr.stCoverExChn.u32Color     = 0x00800080;
        }

        rgn_chn_attr.unChnAttr.stCoverExChn.u32Layer         = i;

        hi_ret = HI_MPI_RGN_AttachToChn(i, &mpp_chn, &rgn_chn_attr);
        if (hi_ret != HI_SUCCESS)
        {
            printf("[%s: %d]HI_MPI_RGN_AttachToChn fail! ret: 0x%x.\n", __func__, __LINE__, hi_ret);
            continue;
        }
    }

    return DEM_SUCCESS;
}


/*******************************************************************************
 * name    : HI_draw_cover_labels
 * function: draw the class labels.
 * input   : bmp_addr:  bmp address
 *           tuple_num: coordinates tuple number
 *           res:       width and height pixel number
 *           cords:     boxes coordinates, such as X1, Y1, X2, Y2
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_draw_cover_labels(char **bmp_addr, int tuple_num, DEM_RES_ST *res, int *cords)
{
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]DEM_cfg_get_global_params fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    if (NULL == bmp_addr || NULL == res)
    {
        printf("[%s: %d]param bmp_addr or res fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;   
    }
#if 0
    if (0 > res->width || 0 > res->height)
    {        
        printf("[%s: %d]resolution is overlimit, width: %d, height: %d!\n",
                 __func__, __LINE__, res->width, res->height);
        return DEM_COMMONFAIL; 
    }
#endif 
    int i = 0;
    int hi_ret = DEM_COMMONFAIL;
    MPP_CHN_S mpp_chn;
    RGN_ATTR_S rgn_attr;
    RGN_CHN_ATTR_S rgn_chn_attr;

    /* Add cover to vpss group */
    mpp_chn.enModId  = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = cfg->osd.attach_venc_id;


    // detach channel first
    for (i = MIN_BMP_RGN_HANDLEID; i < s_rgn_params.max_handle + MIN_BMP_RGN_HANDLEID; i++)
    {
        hi_ret = HI_MPI_RGN_DetachFromChn(i, &mpp_chn);
        if (HI_SUCCESS != hi_ret)
        {
            printf("[%s: %d]HI_MPI_RGN_DetachFromChn %d fail! s32Ret: 0x%x.\n", __func__, __LINE__, i, hi_ret);
            return hi_ret;
        }
    }

    /* Create cover and attach to vpss group */
    int *coords = cords;
    for (i = MIN_BMP_RGN_HANDLEID; i < tuple_num + MIN_BMP_RGN_HANDLEID; i++)
    {
        int label_idx = i - MIN_BMP_RGN_HANDLEID;
        // read the address of coordinates
        coords = cords + 4 * label_idx;
        int x1 = *coords;
        int y1 = *(coords + 1);
        // int x2 = *(coords + 2);
        // int y2 = *(coords + 3);
        int bmp_width  = (res + label_idx)->width;
        int bmp_height = (res + label_idx)->height;

        // set the rgn attributes
        rgn_attr.enType = OVERLAY_RGN;
        rgn_attr.unAttr.stOverlay.enPixelFmt       = PIXEL_FORMAT_RGB_1555;
        rgn_attr.unAttr.stOverlay.stSize.u32Width  = bmp_width;
        rgn_attr.unAttr.stOverlay.stSize.u32Height = bmp_height;
        rgn_attr.unAttr.stOverlay.u32BgColor       = 0x7FFF;        

        // we cann't create a same handle twice
        if (s_rgn_params.max_handle <= label_idx)
        {
            hi_ret = HI_MPI_RGN_Create(i, &rgn_attr);
            if (HI_SUCCESS != hi_ret)
            {
                printf("[%s: %d]HI_MPI_RGN_Create fail! label_idx: %d, handle: %d, width: %d, height: %d, s32Ret: 0x%x.\n",
                     __func__, __LINE__, label_idx, i,
                     rgn_attr.unAttr.stOverlay.stSize.u32Width, rgn_attr.unAttr.stOverlay.stSize.u32Height, hi_ret);
                continue;
            }
        }

        rgn_chn_attr.bShow  = HI_TRUE;
        rgn_chn_attr.enType = OVERLAY_RGN;

        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = x1;
        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = y1 - bmp_height > 0 ? y1 - bmp_height : 0;
        rgn_chn_attr.unChnAttr.stOverlayChn.u32BgAlpha   = 128;
        rgn_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha   = 128;
        rgn_chn_attr.unChnAttr.stOverlayChn.u32Layer     = label_idx;

        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp     = HI_FALSE;
        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp      = 0;
        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;
        
        hi_ret = HI_MPI_RGN_AttachToChn(i, &mpp_chn, &rgn_chn_attr);
        if (hi_ret != HI_SUCCESS)
        {
            printf("[%s: %d]HI_MPI_RGN_AttachToChn fail! handle: %d, ret: 0x%x.\n", __func__, __LINE__, i, hi_ret);
            continue;
        }

        BITMAP_S rgb1555_bmp;
        memset(&rgb1555_bmp, 0, sizeof(BITMAP_S));
        rgb1555_bmp.enPixelFormat = PIXEL_FORMAT_RGB_1555;
        rgb1555_bmp.u32Width      = bmp_width;
        rgb1555_bmp.u32Height     = bmp_height;
        rgb1555_bmp.pData         = (char *)*(unsigned int *)(bmp_addr + label_idx);

        hi_ret = HI_MPI_RGN_SetBitMap(i, &rgb1555_bmp);
        if (hi_ret != HI_SUCCESS)
        {
            printf("[%s: %d]HI_MPI_RGN_SetBitMap fail! handle: %d, ret: 0x%x.\n", __func__, __LINE__, i, hi_ret);
            continue;
        }
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : HI_reflct_coordinate
 * function: mapping the coordinates.
 * input   : tuple_num: coordinates tuple number
 *           src_cord:  boxes coordinates, such as X1, Y1, X2, Y2
 * output  : dest_cord: boxes coordinates, such as X1, Y1, X2, Y2
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_reflct_coordinate(int tuple_num, float *src_cord, int *dest_cord)
{
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]DEM_cfg_get_global_params fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    int vpss_id = cfg->osd.attach_vpss_id;
    // int src_width = cfg->jpeg_res.width;
    // int src_height = cfg->jpeg_res.height;

    int hi_ret = DEM_COMMONFAIL;
    VPSS_GRP group_id = 0;
    VPSS_CHN_MODE_S vpss_chn_mode;
    memset(&vpss_chn_mode, 0, sizeof(vpss_chn_mode));

    hi_ret = HI_MPI_VPSS_GetChnMode(group_id, vpss_id, &vpss_chn_mode);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]HI_MPI_VPSS_GetChnMode fail! ret: 0x%x\n", __func__, __LINE__, hi_ret);
        return hi_ret;        
    }

    int dest_width = vpss_chn_mode.u32Width;
    int dest_height = vpss_chn_mode.u32Height;

    int loop = 0;
    int *addr = dest_cord;
    int x1, y1, x2, y2;
    float src_x1, src_y1, src_x2, src_y2;
    float *src_addr = src_cord;
    for (loop = 0; loop < tuple_num; ++loop)
    {
        src_x1 = *src_addr;
        src_y1 = *(src_addr + 1);
        src_x2 = *(src_addr + 2);
        src_y2 = *(src_addr + 3);
        //printf("[%s: %d]Start Reflect coordinates: x1: %f, y1: %f,  x2: %f, y2: %f\n", __func__, __LINE__,
                //src_x1, src_y1, src_x2, src_y2);

        x1 = (((int)(src_x1 * dest_width )) >> 1) << 1;
        y1 = (((int)(src_y1 * dest_height)) >> 1) << 1;
        x2 = (((int)(src_x2 * dest_width )) >> 1) << 1;
        y2 = (((int)(src_y2 * dest_height)) >> 1) << 1;

        // printf("[%s: %d]Reflect coordinates: box index: %d, x1: %d, y1: %d, x2: %d, y2: %d\n", 
        //         __func__, __LINE__, loop, x1, y1, x2, y2);

        *(addr++) = x1 > 0 ? x1 : 0;
        *(addr++) = y1 > 0 ? y1 : 32;
        *(addr++) = x2 < dest_width ? x2 : dest_width;
        *(addr++) = y2 < dest_height ? y2 : dest_height;

        src_addr += 4;
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : HI_wrap_vi_ch_check
 * function: check vi channel number by vi channel number.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_vi_ch_check(int channel_num)
{
#if ARCH != arm
    return DEM_SUCCESS;
#else
    int hi_ret = DEM_COMMONFAIL;

    // get vi fd
    int hi_fd = -1;

    hi_fd = HI_MPI_VI_GetFd(channel_num);
    if (hi_fd <= 0)
    {
        printf("[%s: %d]HI_MPI_VI_GetFd fail! ret: 0x%x\n", __func__, __LINE__, hi_ret);
        return DEM_COMMONFAIL;
    }
    printf("[%s: %d]HI_MPI_VI_GetFd success! ViChannel: %d, fd: %d\n", __func__, __LINE__, channel_num, hi_fd);

    // get vi channel attribute
    VI_CHN_ATTR_S channel_attr;
    memset(&channel_attr, 0, sizeof(VI_CHN_ATTR_S));
    hi_ret = HI_MPI_VI_GetChnAttr(channel_num, &channel_attr);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]HI_MPI_VI_GetChnAttr ch%d fail! ret: 0x%x\n", __func__, __LINE__, channel_num, hi_ret);
        return DEM_COMMONFAIL;
    }

#if 0   // HI_MPI_VI_GetChnBind is unsupport
    VI_CHN_BIND_ATTR_S channel_bind_attr;
    memset(&channel_bind_attr, 0, sizeof(VI_CHN_BIND_ATTR_S));

    hi_ret = HI_MPI_VI_GetChnBind(channel_num, &channel_bind_attr);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]HI_MPI_VI_GetChnBind fail! ret: 0x%x\n", __func__, __LINE__, hi_ret);
        // return DEM_COMMONFAIL;
    }

    printf("[%s: %d]HI_MPI_VI_GetChnBind success! ViDev: %d, ViWay: %d\n", __func__, __LINE__,
           channel_bind_attr.ViDev, channel_bind_attr.ViWay);
#endif

    return DEM_SUCCESS;
#endif  /* ARCH != arm */
}

/*******************************************************************************
 * name    : HI_wrap_jpegqua_set
 * function: set a jpeg quality.
 * input   : jpeg_channel_id:  channel id of jpeg encodec
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_jpegqua_set(int jpeg_channel_id)
{
    int hi_ret = DEM_COMMONFAIL;
    int quality = 80;

    VENC_PARAM_JPEG_S jpeg_params;
    memset(&jpeg_params, 0, sizeof(VENC_PARAM_JPEG_S));

    hi_ret = HI_MPI_VENC_GetJpegParam(jpeg_channel_id, &jpeg_params);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] jpeg channel %d, HI_MPI_VENC_GetJpegParam fail: 0x%x\n", __func__, __LINE__,
            jpeg_channel_id, hi_ret);
        return hi_ret;
    }

    jpeg_params.u32Qfactor = quality;

    hi_ret = HI_MPI_VENC_SetJpegParam(jpeg_channel_id, &jpeg_params);
    if (SUCCESS != hi_ret)
    {
        printf("[%s: %d] jpeg channel %d, HI_MPI_VENC_SetJpegParam fail: 0x%x\n", __func__, __LINE__,
            jpeg_channel_id, hi_ret);
        return HI_FAILURE;
    }

    printf("[%s: %d] end, jpeg channel %d set quality %d success\n", __func__, __LINE__, jpeg_channel_id, quality);
    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : HI_wrap_vpsschn_start
 * function: start a vpss channel.
 * input   : vpss_group_id: group id of vpss
 *           vpss_channel_id:  channel id of vpss
 *           chn_attr: vpss channel attribute
 *           width: encode jpeg picture witdh
 *           height: encode jpeg picture height
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_vpsschn_start(int vpss_group_id, int vpss_channel_id, VPSS_CHN_ATTR_S *chn_attr, int width, int height)
{
    int hi_ret = DEM_COMMONFAIL;
    VPSS_CHN_ATTR_S vpss_chn_attr;
    SIZE_S tSize;
    VPSS_CHN_MODE_S vpss_chn_mode;

    memset(&vpss_chn_attr, 0, sizeof(VPSS_CHN_ATTR_S));

    if ((vpss_group_id < 0) || (vpss_group_id >= VPSS_MAX_GRP_NUM))
    {
        printf("[%s: %d] vpss_group_id %d invalid\n", __func__, __LINE__, vpss_group_id);
        return DEM_VPSSGRPFAIL;
    }

    if ((vpss_channel_id < 0) || (vpss_channel_id >= VPSS_MAX_CHN_NUM))
    {
        printf("[%s: %d] vpss_channel_id %d invalid\n", __func__, __LINE__, vpss_channel_id);
        return DEM_VPSSCHNFAIL;
    }

    /* set vpss channel attribute. */
    if (NULL == chn_attr)
    {
        vpss_chn_attr.bSpEn = DEM_FALSE;
        vpss_chn_attr.bBorderEn = DEM_FALSE;
        vpss_chn_attr.s32DstFrameRate = -1;
        vpss_chn_attr.s32SrcFrameRate = -1;
    }
    else
    {
        memcpy(&vpss_chn_attr, chn_attr, sizeof(VPSS_CHN_ATTR_S));
    }

    /* set vpss attribute. */
    printf("[%s: %d] start to HI_MPI_VPSS_Sevpss_chn_attr, vpss group: %d, vpss channel: %d\n",
        __func__, __LINE__, vpss_group_id, vpss_channel_id);
    hi_ret = HI_MPI_VPSS_SetChnAttr(vpss_group_id, vpss_channel_id, &vpss_chn_attr);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] HI_MPI_VPSS_Sevpss_chn_attr fail, vpss group: %d, vpss channel: %d, err: 0x%08x\n",
            __func__, __LINE__, vpss_group_id, vpss_channel_id, hi_ret);
        return hi_ret;
    }

    memset(&vpss_chn_mode, 0, sizeof(VPSS_CHN_MODE_S));

    hi_ret = HI_MPI_VPSS_GetChnMode(vpss_group_id, vpss_channel_id, &vpss_chn_mode);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]HI_MPI_VPSS_GetChnMode fail, vpss group: %d, vpss channel: %d, err: 0x%08x\n",
            __func__, __LINE__, vpss_group_id, vpss_channel_id, hi_ret);
        return hi_ret;
    }

    printf("[%s: %d]Get vpss vpss channel: %d, vpss_chn_mode.u32Width: %d, vpss_chn_mode.u32Height: %d.\n",
                __func__, __LINE__, vpss_channel_id, vpss_chn_mode.u32Width,  vpss_chn_mode.u32Height);

    vpss_chn_mode.enChnMode = VPSS_CHN_MODE_USER;
    vpss_chn_mode.bDouble = FALSE;
    vpss_chn_mode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    if (vpss_channel_id != VPSS_ALG_CHN)
    {
        vpss_chn_mode.enCompressMode = COMPRESS_MODE_SEG;
    }
    else
    {
        vpss_chn_mode.enCompressMode = COMPRESS_MODE_NONE;
    }

    printf("[%s: %d]Set vpss channel: %d, tSize.u32Width: %d, tSize.u32Height: %d.\n",
                __func__, __LINE__, vpss_channel_id, tSize.u32Width,  tSize.u32Height);

    vpss_chn_mode.u32Width = width;
    vpss_chn_mode.u32Height = height;

    hi_ret = HI_MPI_VPSS_SetChnMode(vpss_group_id, vpss_channel_id, &vpss_chn_mode);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]Set vpss chn mode failed. err: 0x%x.\n",__func__, __LINE__, hi_ret);
        return hi_ret;
    }

    /* enable vpss channel. */
    hi_ret = HI_MPI_VPSS_EnableChn(vpss_group_id, vpss_channel_id);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d]Enable VPSS channal fail. err: 0x%x.\n",__func__, __LINE__, hi_ret);
        return hi_ret;
    }


    printf("[%s: %d] end, vpss channel %d start success\n", __func__, __LINE__, vpss_channel_id);
    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : HI_wrap_jpeg_encodec_create
 * function: create a jpeg encodec.
 * input   : vi_channel_id: channel id of vi
 *           jpeg_channel_id:  channel id of jpeg encodec
 *           width: encode jpeg picture witdh
 *           height: encode jpeg picture height
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_jpeg_encodec_create(int vi_channel_id, int jpeg_channel_id, int width, int height)
{
    int hi_ret = DEM_COMMONFAIL;

    int vpss_group_id = 0;
    int vpss_channel_id = 1;
    int vi_dev_id = 0;


    if (vi_channel_id < 0 || vi_channel_id > VIU_MAX_DEV_NUM)
    {
        printf("[%s: %d] Video input channal is overflow.\n", __func__, __LINE__);
        return DEM_VICHNFAIL;
    }

    if (jpeg_channel_id < 0)
    {
        printf("[%s: %d] JPEG encode channal is overflow.\n", __func__, __LINE__);
        return DEM_JPEGCHNFAIL;
    }

    if (0 >= width || 0 >= height)
    {
        printf("[%s: %d] width or height is error, width: %d, height: %d.\n", __func__, __LINE__, width, height);
        return DEM_RESFAIL;
    }

    /* start to encode channel. */
    printf("[%s: %d] Start encode channel.\n", __func__, __LINE__);
    VENC_CHN_ATTR_S venc_chn_attr;
    VENC_ATTR_JPEG_S jpeg_chn_attr;
    jpeg_chn_attr.u32MaxPicWidth  = width;
    jpeg_chn_attr.u32MaxPicHeight = height;
    jpeg_chn_attr.u32PicWidth     = width;
    jpeg_chn_attr.u32PicHeight    = height;
    jpeg_chn_attr.u32BufSize      = width * height * 2;
    jpeg_chn_attr.bByFrame        = DEM_TRUE;   /*get stream mode is field mode or frame mode*/
    jpeg_chn_attr.bSupportDCF     = DEM_FALSE;

    venc_chn_attr.stVeAttr.enType = PT_JPEG;
    memcpy(&venc_chn_attr.stVeAttr.stAttrJpeg, &jpeg_chn_attr, sizeof(VENC_ATTR_JPEG_S));

    printf("*****************************Chn enc attr***********************************\n");
    printf("-------------tVencChnAttr.stVeAttr.enType=%d--------\n", venc_chn_attr.stVeAttr.enType);
    printf("tVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = %d\n", venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth);
    printf("tVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = %d\n", venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight);
    printf("tVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth = %d\n", venc_chn_attr.stVeAttr.stAttrH264e.u32PicWidth);
    printf("tVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight = %d\n", venc_chn_attr.stVeAttr.stAttrH264e.u32PicHeight);
    printf("tVencChnAttr.stRcAttr.enRcMode = %d\n", venc_chn_attr.stRcAttr.enRcMode);

    printf("tVencChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = %d\n", venc_chn_attr.stVeAttr.stAttrJpeg.u32MaxPicWidth);
    printf("tVencChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = %d\n", venc_chn_attr.stVeAttr.stAttrJpeg.u32MaxPicHeight);
    printf("tVencChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = %d\n", venc_chn_attr.stVeAttr.stAttrJpeg.u32PicWidth);
    printf("tVencChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = %d\n", venc_chn_attr.stVeAttr.stAttrJpeg.u32PicHeight);

    printf("**************************************************************************\n");

    /* create venc channel. */
    hi_ret = HI_MPI_VENC_CreateChn(jpeg_channel_id, &venc_chn_attr);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Create video encode channal %d fail (err: 0x%08x).\n",  __func__, __LINE__,
            jpeg_channel_id, hi_ret);
        return hi_ret;
    }

    /* set jpeg quality */
    HI_wrap_jpegqua_set(jpeg_channel_id);

    /* start encode channel to recieve pictures. */
    hi_ret = HI_MPI_VENC_StartRecvPic(jpeg_channel_id);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Start VENC picture recieve fail (err: 0x%08x).\n", __func__, __LINE__, hi_ret);
        goto STOPENC;
    }

    /* set buffer size for encode jpeg, this setting just save 1 picture */
    hi_ret = HI_MPI_VENC_SetMaxStreamCnt(jpeg_channel_id, 1);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Set max stream count in buffer failed (err: 0x%08x).\n", __func__, __LINE__, hi_ret);
        goto STOPENC;
    }

    printf("[%s: %d] Start vpss channel.\n", __func__, __LINE__);
    hi_ret = HI_wrap_vpsschn_start(vi_channel_id, vpss_channel_id, NULL, width, height);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] HI_wrap_vpsschn_start failed.\n", __func__, __LINE__);
        goto STOPENC;
    }

    /* bind vpss channel to encode channel. */
    printf("[%s: %d] Start to bind vpss to encode channel.\n", __func__, __LINE__);
    MPP_CHN_S mpp_src_chn;
    MPP_CHN_S mpp_dst_chn;

    mpp_src_chn.enModId  = HI_ID_VPSS;
    mpp_src_chn.s32DevId = vpss_group_id;
    mpp_src_chn.s32ChnId = vpss_channel_id;

    mpp_dst_chn.enModId  = HI_ID_VENC;
    mpp_dst_chn.s32DevId = vi_dev_id;
    mpp_dst_chn.s32ChnId = jpeg_channel_id;
    hi_ret = HI_MPI_SYS_Bind(&mpp_src_chn, &mpp_dst_chn);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Bind Video encode channal to VPSS channal fail. vpss:(group: %d, channel: %d), venc(channel: %d)\n", 
                __func__, __LINE__, vpss_group_id, vpss_channel_id, jpeg_channel_id);
        goto STOPENC;
    }

    return DEM_SUCCESS;

STOPENC:

    HI_MPI_VENC_StopRecvPic(jpeg_channel_id);
    HI_MPI_VENC_DestroyChn(jpeg_channel_id);

    return DEM_COMMONFAIL;
}

/*******************************************************************************
 * name    : HI_wrap_jpeg_cap_get
 * function: capture a jpeg package.
 * input   : enc_ch: encode channel id for capture
 *           max_buf_len:  max buffer length for save
 *           lock:      lock
 * output  : out_buf:   buffer for save picture
 *           out_len:   buffer length for save picture
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_jpeg_cap_get(int enc_ch, int max_buf_len, pthread_mutex_t *lock, char *out_buf, int *out_len)
#if 0
{
    int hi_ret = DEM_COMMONFAIL;
    VENC_CHN_STAT_S venc_chn_state;
    VENC_STREAM_S venc_stream;
    VENC_PACK_S *venc_packs;

    *out_len = 0;
    /* query enc state */
    //printf("[%s: %d] HI_MPI_VENC_Query chn %d start.\n", __func__, __LINE__, enc_ch);
    hi_ret = HI_MPI_VENC_Query(enc_ch, &venc_chn_state);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Query venc chn %d fail.\n", __func__, __LINE__, enc_ch);
        return hi_ret;
    }

    if (venc_chn_state.u32CurPacks <= 0)
    {
        printf("[%s: %d] chn[%d] curpacks get failed.\n", __func__, __LINE__, enc_ch);
        return DEM_COMMONFAIL;
    }

    /* get a frame */
    venc_stream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * venc_chn_state.u32CurPacks);
    if (NULL == venc_stream.pstPack)
    {
        printf("[%s: %d] chn[%d] malloc venc_stream.pstPack fail.\n", __func__, __LINE__, enc_ch);
        return DEM_COMMONFAIL;
    }

    //printf("[%s: %d] HI_MPI_VENC_GetStream chn %d start.\n", __func__, __LINE__, enc_ch);
    venc_stream.u32PackCount = venc_chn_state.u32CurPacks;
    hi_ret = HI_MPI_VENC_GetStream(enc_ch, &venc_stream, HI_TRUE);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] Get video stream failed.\n", __func__, __LINE__);
        hi_ret = DEM_COMMONFAIL;
        goto FREEPACK;
    }


    /* copy picture */
    int i = 0;
    int save_size = 0;
    int write_size = 0;

    // lock mutex
    if (NULL != lock)
    {
        pthread_mutex_lock(lock);
    }
    for (i = 0; i < venc_stream.u32PackCount; i++)
    {
        venc_packs = &(venc_stream.pstPack[i]);
        write_size = venc_packs->u32Len - venc_packs->u32Offset;
        if (save_size + write_size > max_buf_len)
        {
            printf("[%s: %d] picture data size is too large, i: %d, size: %d.\n", __func__, __LINE__, i, save_size + write_size);
            hi_ret = DEM_COMMONFAIL;
            break;
        }
        else
        {
            memcpy(out_buf + save_size, venc_packs->pu8Addr + venc_packs->u32Offset, write_size);
            save_size += write_size;
            hi_ret = DEM_SUCCESS;
            //printf("[%s: %d] get picture chn %d success. id: %d len: %d\n", __func__, __LINE__, enc_ch, i, write_size);
        }

        //fwrite(venc_packs->pu8Addr + venc_packs->u32Offset, sizeof(u8), venc_packs->u32Len - venc_packs->u32Offset, pFile);
    }

    if (DEM_SUCCESS == hi_ret)
    {
        *out_len = save_size;
        //printf("[%s: %d] get picture chn %d success. picture len: %d\n", __func__, __LINE__, enc_ch, *out_len);
    }

    // unlock mutex
    if (NULL != lock)
    {
        pthread_mutex_unlock(lock);
    }

FREEPACK:
    if (NULL != venc_stream.pstPack)
    {
        HI_MPI_VENC_ReleaseStream(enc_ch, &venc_stream);

        free(venc_stream.pstPack);
        venc_stream.pstPack = NULL;
    }
    return hi_ret;
}
#else
// capture YUV from vpss channel and then user for IVE YUV2RGB
// use for vpss channel 1
{
    int hi_ret   = DEM_COMMONFAIL;
    int group    = 0;
    int vpss_chn = 1;
    int wait     = 10;      // 10ms

    HI_U32 org_dep = 0;
    VIDEO_FRAME_INFO_S fram_info;

    //printf("[%s: %d] HI_MPI_VENC_Query chn %d start.\n", __func__, __LINE__, enc_ch);
    if (DEM_SUCCESS != (hi_ret = HI_MPI_VPSS_GetDepth(group, vpss_chn, &org_dep)))
    {
        printf("[%s: %d] HI_MPI_VPSS_GetDepth failed. errno: 0x%08x\n", __func__, __LINE__, hi_ret);
        return hi_ret;
    }

    if (1 != org_dep)
    {
        if (DEM_SUCCESS != (hi_ret = HI_MPI_VPSS_SetDepth(group, vpss_chn, 1)))
        {
            printf("[%s: %d] HI_MPI_VPSS_SetDepth failed. errno: 0x%08x\n", __func__, __LINE__, hi_ret);
            HI_MPI_VPSS_SetDepth(group, vpss_chn, org_dep);
            return hi_ret;
        }
    }

    memset(&fram_info, 0, sizeof(VIDEO_FRAME_INFO_S));
    fram_info.u32PoolId = VB_INVALID_POOLID;
    if (DEM_SUCCESS != (hi_ret = HI_MPI_VPSS_GetChnFrame(group, vpss_chn, &fram_info, wait)))
    {
        printf("[%s: %d] HI_MPI_VPSS_GetChnFrame failed. errno: 0x%08x\n", __func__, __LINE__, hi_ret);
        return DEM_SUCCESS;
    }

    #if 0
    printf("-----------------------------------------------------\n");
    printf("fram_info.stVFrame.u32Width: %u\n", fram_info.stVFrame.u32Width);
    printf("fram_info.stVFrame.u32Stride[0]: %u\n", fram_info.stVFrame.u32Stride[0]);
    printf("fram_info.stVFrame.u32Stride[1]: %u\n", fram_info.stVFrame.u32Stride[1]);
    printf("fram_info.stVFrame.u32Stride[2]: %u\n", fram_info.stVFrame.u32Stride[2]);
    printf("fram_info.stVFrame.u32PhyAddr[0]: 0x%x\n", fram_info.stVFrame.u32PhyAddr[0]);
    printf("fram_info.stVFrame.u32PhyAddr[1]: 0x%x\n", fram_info.stVFrame.u32PhyAddr[1]);
    printf("fram_info.stVFrame.u32PhyAddr[2]: 0x%x\n", fram_info.stVFrame.u32PhyAddr[2]);
    printf("fram_info.stVFrame.pVirAddr[0]: 0x%x\n", fram_info.stVFrame.pVirAddr[0]);
    printf("fram_info.stVFrame.pVirAddr[1]: 0x%x\n", fram_info.stVFrame.pVirAddr[1]);
    printf("fram_info.stVFrame.pVirAddr[2]: 0x%x\n", fram_info.stVFrame.pVirAddr[2]);
    printf("-----------------------------------------------------\n");
    #endif

    IVE_IMAGE_TYPE_E image_type = IVE_IMAGE_TYPE_BUTT;
    switch (fram_info.stVFrame.enPixelFormat)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            image_type = IVE_IMAGE_TYPE_YUV420SP;
            break;
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
            image_type = IVE_IMAGE_TYPE_YUV422SP;
            break;
        default:
            break;
    }

    if (IVE_IMAGE_TYPE_BUTT == image_type)
    {
        printf("[%s: %d] image_type is error. type: %d\n", __func__, __LINE__, fram_info.stVFrame.enPixelFormat);

        HI_MPI_VPSS_SetDepth(group, vpss_chn, org_dep);
        hi_ret = DEM_COMMONFAIL;
        goto REL_FRAME;
    }

    /* convert YUV2RGB */
    HI_VOID *pVirtDst = NULL;
    IVE_SRC_IMAGE_S stSrc;
    IVE_DST_IMAGE_S stDst;
    unsigned int   *pImage = NULL;

    stSrc.enType        = image_type;
    stSrc.u16Width      = fram_info.stVFrame.u32Width;
    stSrc.u16Height     = fram_info.stVFrame.u32Height;
    stSrc.u16Stride[0]  = fram_info.stVFrame.u32Stride[0];
    stSrc.u16Stride[1]  = fram_info.stVFrame.u32Stride[1];
    stSrc.u16Stride[2]  = fram_info.stVFrame.u32Stride[2];
    stSrc.u32PhyAddr[0] = fram_info.stVFrame.u32PhyAddr[0];
    stSrc.u32PhyAddr[1] = fram_info.stVFrame.u32PhyAddr[1];
    stSrc.u32PhyAddr[2] = fram_info.stVFrame.u32PhyAddr[2];
    stSrc.pu8VirAddr[0] = fram_info.stVFrame.pVirAddr[0];
    stSrc.pu8VirAddr[1] = fram_info.stVFrame.pVirAddr[1];
    stSrc.pu8VirAddr[2] = fram_info.stVFrame.pVirAddr[2];

    stDst.enType        = IVE_IMAGE_TYPE_U8C3_PACKAGE;      // BGR
    stDst.u16Width      = stSrc.u16Width;
    stDst.u16Height     = stSrc.u16Height;
    stDst.u16Stride[0]  = stSrc.u16Stride[0];
    stDst.u16Stride[1]  = stSrc.u16Stride[1];
    stDst.u16Stride[2]  = stSrc.u16Stride[2];

    IVE_HANDLE IveHandle;
    HI_BOOL bInstant = HI_TRUE;
    IVE_CSC_CTRL_S stCscCtrl;
    stCscCtrl.enMode = IVE_CSC_MODE_PIC_BT601_YUV2RGB;

    hi_ret = HI_MPI_SYS_MmzAlloc_Cached(
        &stDst.u32PhyAddr[0], &pVirtDst, "User", HI_NULL, stSrc.u16Height * stDst.u16Stride[0]);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] HI_MPI_SYS_MmzAlloc_Cached fail. errno: 0x%08x\n", __func__, __LINE__, hi_ret);
        HI_MPI_VPSS_SetDepth(group, vpss_chn, org_dep);
        goto REL_FRAME;
    }
    #if 0
    printf("-------------------After MMZ Alloc--------------------\n");
    printf("stSrc.u16Width: %u\n", stSrc.u16Width);
    printf("stSrc.u16Stride[0]: %u\n", stSrc.u16Stride[0]);
    printf("stSrc.u16Stride[1]: %u\n", stSrc.u16Stride[1]);
    printf("stSrc.u16Stride[2]: %u\n", stSrc.u16Stride[2]);
    printf("stSrc.u32PhyAddr[0]: 0x%x\n", stSrc.u32PhyAddr[0]);
    printf("stSrc.u32PhyAddr[1]: 0x%x\n", stSrc.u32PhyAddr[1]);
    printf("stSrc.u32PhyAddr[2]: 0x%x\n", stSrc.u32PhyAddr[2]);
    printf("stSrc.pu8VirAddr[0]: 0x%x\n", stSrc.pu8VirAddr[0]);
    printf("stSrc.pu8VirAddr[1]: 0x%x\n", stSrc.pu8VirAddr[1]);
    printf("stSrc.pu8VirAddr[2]: 0x%x\n", stSrc.pu8VirAddr[2]);
    printf("-----------------------------------------------------\n");
    #endif

    HI_MPI_SYS_MmzFree(stDst.u32PhyAddr[0], pVirtDst);
    #if 0
    printf("-------------------After MMZ free----------------------\n");
    printf("stSrc.u16Width: %u\n", stSrc.u16Width);
    printf("stSrc.u16Stride[0]: %u\n", stSrc.u16Stride[0]);
    printf("stSrc.u16Stride[1]: %u\n", stSrc.u16Stride[1]);
    printf("stSrc.u16Stride[2]: %u\n", stSrc.u16Stride[2]);
    printf("stSrc.u32PhyAddr[0]: 0x%x\n", stSrc.u32PhyAddr[0]);
    printf("stSrc.u32PhyAddr[1]: 0x%x\n", stSrc.u32PhyAddr[1]);
    printf("stSrc.u32PhyAddr[2]: 0x%x\n", stSrc.u32PhyAddr[2]);
    printf("stSrc.pu8VirAddr[0]: 0x%x\n", stSrc.pu8VirAddr[0]);
    printf("stSrc.pu8VirAddr[1]: 0x%x\n", stSrc.pu8VirAddr[1]);
    printf("stSrc.pu8VirAddr[2]: 0x%x\n", stSrc.pu8VirAddr[2]);
    printf("-----------------------------------------------------\n");
    #endif

    hi_ret = HI_MPI_IVE_CSC(&IveHandle, &stSrc, &stDst, &stCscCtrl, bInstant);
    if (DEM_SUCCESS != hi_ret)
    {
        printf("[%s: %d] HI_MPI_IVE_CSC fail. errno: 0x%08x\n", __func__, __LINE__, hi_ret);
        HI_MPI_VPSS_SetDepth(group, vpss_chn, org_dep);
        goto REL_FRAME;
    }

    pImage = HI_MPI_SYS_Mmap(stDst.u32PhyAddr[0], (HI_U32)(stDst.u16Width * stDst.u16Height * 3));
    if (NULL == pImage)
    {
        printf("[%s: %d] HI_MPI_SYS_Mmap fail.\n", __func__, __LINE__);
        hi_ret = DEM_COMMONFAIL;
        HI_MPI_VPSS_SetDepth(group, vpss_chn, org_dep);
        goto REL_FRAME;
    }

    /* copy picture */
    unsigned int save_size = stDst.u16Width * stDst.u16Height * 3;
    save_size = max_buf_len > save_size ? save_size : max_buf_len;

    // lock mutex
    if (NULL != lock)
    {
        pthread_mutex_lock(lock);
    }

    *out_len = 0;
    memcpy(out_buf, pImage, save_size);
    *out_len = save_size;
    //printf("[%s: %d] get picture chn %d success. picture len: %d\n", __func__, __LINE__, enc_ch, *out_len);

    // unlock mutex
    if (NULL != lock)
    {
        pthread_mutex_unlock(lock);
    }

    HI_MPI_SYS_Munmap(pImage, save_size);
    // release frame
REL_FRAME:
    HI_MPI_VPSS_ReleaseChnFrame(group, vpss_chn, &fram_info);
    // printf("[%s: %d] finish! result: 0x%x\n", __func__, __LINE__, hi_ret);

    return hi_ret;
}
#endif
/*******************************************************************************
 * name    : HI_wrap_label_box_show
 * function: attach labels and boxes on venc.
 * input   : number:      osd boxes number
 *           label_bmp:   labels bmp
 *           bmp_res:     bmp resolution
 *           boxes:       boxes coordinates, such as X1, Y1, X2, Y2
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_label_box_show(int number, char **label_bmp, DEM_RES_ST *bmp_res, float *boxes)
{
    if (0 == number)        // clear all boxes
    {
        int rgn_handle = 0;
        for (rgn_handle = 0; rgn_handle < s_rgn_params.max_handle; ++rgn_handle)
        {
            HI_MPI_RGN_Destroy(rgn_handle);
        }

        // Destory bmp handle
        for (rgn_handle = MIN_BMP_RGN_HANDLEID; 
            rgn_handle < MIN_BMP_RGN_HANDLEID + s_rgn_params.max_handle; ++rgn_handle)
        {
            HI_MPI_RGN_Destroy(rgn_handle);
        }

        s_rgn_params.max_handle = 0;
        return DEM_SUCCESS;
    }

    if (NULL == label_bmp || NULL == boxes || NULL == bmp_res)
    {
        printf("[%s: %d] label_bmp or boxes is NULL.\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    int reflct_boxes[number][4];
    int ret = DEM_COMMONFAIL;
    // get all the coordinate reflct
    ret = HI_reflct_coordinate(number, boxes, &reflct_boxes[0][0]);
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d] HI_reflct_coordinate fail.\n", __func__, __LINE__);
        return ret;
    }

    // draw the boxes
    ret = HI_draw_cover_boxes(number, &reflct_boxes[0][0]);
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d] HI_draw_cover_boxes fail.\n", __func__, __LINE__);
        return ret;
    }

    // draw the labels
    ret = HI_draw_cover_labels(label_bmp, number, bmp_res, &reflct_boxes[0][0]);
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d] HI_draw_cover_labels fail.\n", __func__, __LINE__);
        return ret;
    }

    s_rgn_params.max_handle = number > s_rgn_params.max_handle ? number : s_rgn_params.max_handle;
    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : HI_wrap_osd_fixtype
 * function: attach osd on venc.
 * input   : 
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_osd_fixtype(void)
{
    HI_S32 handle = 0;
    HI_S32 u32Num = 4;
    HI_S32 i = 0;
    HI_S32 s32Ret;
    MPP_CHN_S stChn;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr;

    /* Add cover to vpss group */
    stChn.enModId  = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 3;

    /* Create cover and attach to vpss group */
    for (i = handle; i < (handle + u32Num); i++)
    {
        stRgnAttr.enType = COVEREX_RGN;

        s32Ret = HI_MPI_RGN_Create(i, &stRgnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_RGN_Create fail! s32Ret: 0x%x.\n", s32Ret);
            return s32Ret;
        }

        stChnAttr.bShow  = HI_TRUE;
        stChnAttr.enType = COVEREX_RGN;

        stChnAttr.unChnAttr.stCoverExChn.enCoverType = AREA_QUAD_RANGLE;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.bSolid = HI_FALSE;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.u32Thick = 2;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[0].s32X = 50 * i;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[0].s32Y = 0;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[1].s32X = 50 + 50 * i;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[1].s32Y = 50;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[2].s32X = 50 + 50 * i;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[2].s32Y = 300;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[3].s32X = 50 * i;
        stChnAttr.unChnAttr.stCoverExChn.stQuadRangle.stPoint[3].s32Y = 200;

        stChnAttr.unChnAttr.stCoverExChn.u32Color         = 0x000000ff;
        if (1 == i % COVEREX_MAX_NUM_VPSS)
        {
            stChnAttr.unChnAttr.stCoverExChn.u32Color     = 0x0000ff00;
        }
        else if (2 == i % COVEREX_MAX_NUM_VPSS)
        {
            stChnAttr.unChnAttr.stCoverExChn.u32Color     = 0x00ff0000;
        }
        else if (3 == i % COVEREX_MAX_NUM_VPSS)
        {
            stChnAttr.unChnAttr.stCoverExChn.u32Color     = 0x00808080;
        }
        stChnAttr.unChnAttr.stCoverExChn.u32Layer         = i - handle;

        s32Ret = HI_MPI_RGN_AttachToChn(i, &stChn, &stChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_RGN_AttachToChn fail! handle: %d, s32Ret: 0x%x.\n", i, s32Ret);
            return s32Ret;
        }
    }

    return DEM_SUCCESS;

}

/*******************************************************************************
 * name    : HI_wrap_draw_cover_labels_fix
 * function: draw the class labels for test.
 * input   : 
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_draw_cover_labels_fix(char *bmp_addr)
{
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]DEM_cfg_get_global_params fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    if (NULL == bmp_addr)
    {
        printf("[%s: %d]param bmp_addr or res fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;   
    }
    
    int i = 0;
    int hi_ret = DEM_COMMONFAIL;
    MPP_CHN_S mpp_chn;
    RGN_ATTR_S rgn_attr;
    RGN_CHN_ATTR_S rgn_chn_attr;

    /* Add cover to vpss group */
    mpp_chn.enModId  = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = cfg->osd.attach_venc_id;

    printf("[%s: %d]start to create overlay rgn on encode channel: %d!\n", __func__, __LINE__, mpp_chn.s32ChnId);

    /* Create cover and attach to vpss group */
    for (i = MIN_BMP_RGN_HANDLEID; i < 1 + MIN_BMP_RGN_HANDLEID; i++)
    {
        printf("[%s: %d]start to create rgn handle: %d!\n", __func__, __LINE__, i);
        // set the rgn attributes
        rgn_attr.enType = OVERLAY_RGN;
        rgn_attr.unAttr.stOverlay.enPixelFmt       = PIXEL_FORMAT_RGB_1555;
        rgn_attr.unAttr.stOverlay.stSize.u32Width  = 64;
        rgn_attr.unAttr.stOverlay.stSize.u32Height = 32;
        rgn_attr.unAttr.stOverlay.u32BgColor       = 0x7FFF;

        // we cann't create a same handle twice
        hi_ret = HI_MPI_RGN_Create(i, &rgn_attr);
        if (HI_SUCCESS != hi_ret)
        {
            printf("[%s: %d]HI_MPI_RGN_Create fail! s32Ret: 0x%x.\n", __func__, __LINE__, hi_ret);
            break;
        }
       

        rgn_chn_attr.bShow  = HI_TRUE;
        rgn_chn_attr.enType = OVERLAY_RGN;

        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = 100 + 50 * (i - MIN_BMP_RGN_HANDLEID);
        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = 100 + 50 * (i - MIN_BMP_RGN_HANDLEID);
        rgn_chn_attr.unChnAttr.stOverlayChn.u32BgAlpha   = 128;
        rgn_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha   = 128;
        rgn_chn_attr.unChnAttr.stOverlayChn.u32Layer     = i - MIN_BMP_RGN_HANDLEID;
        
        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp     = HI_FALSE;
        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp      = 0;
        rgn_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;

        // rgn_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16 * (i % 2 + 1);
        // rgn_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width  = 16 * (i % 2 + 1);
        // rgn_chn_attr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
        // rgn_chn_attr.unChnAttr.stOverlayChn.stInvertColor.enChgMod     = LESSTHAN_LUM_THRESH;
        // rgn_chn_attr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn    = HI_FALSE;
        
        hi_ret = HI_MPI_RGN_AttachToChn(i, &mpp_chn, &rgn_chn_attr);
        if (hi_ret != HI_SUCCESS)
        {
            printf("[%s: %d]HI_MPI_RGN_AttachToChn fail! handle: %d, ret: 0x%x.\n", __func__, __LINE__, i, hi_ret);
            break;
        }

        BITMAP_S rgb1555_bmp;
        memset(&rgb1555_bmp, 0, sizeof(BITMAP_S));
        rgb1555_bmp.enPixelFormat = PIXEL_FORMAT_RGB_1555;
        rgb1555_bmp.u32Width      = 16;
        rgb1555_bmp.u32Height     = 16;
        rgb1555_bmp.pData = (char *)*(unsigned int *)(bmp_addr + i - MIN_BMP_RGN_HANDLEID);
        // printf("[%s: %d]start to HI_MPI_RGN_SetBitMap, address is 0x%x!\n", __func__, __LINE__, (char *)rgb1555_bmp.pData);

        hi_ret = HI_MPI_RGN_SetBitMap(i, &rgb1555_bmp);
        if (hi_ret != HI_SUCCESS)
        {
            printf("[%s: %d]HI_MPI_RGN_SetBitMap fail! ret: 0x%x.\n", __func__, __LINE__, hi_ret);
            break;
        }
    }

    return DEM_SUCCESS;
}