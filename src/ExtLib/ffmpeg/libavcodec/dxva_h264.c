/*
 * (C) 2016 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <windows.h>
#include "dxva_internal.h"
#include "dxva_h264.h"

void h264_getcurframe(struct AVCodecContext* avctx, AVFrame** frame)
{
    const H264Context *h = avctx->priv_data;
    *frame               = h->cur_pic.f;
}

static void fill_picture_entry(DXVA_PicEntry_H264 *pic,
                               unsigned index, unsigned flag)
{
    assert((index&0x7f) == index && (flag&0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}

static void fill_picture_parameters(const H264Context *h,
                                    dxva_context *ctx,
                                    DXVA_PicParams_H264 *pp)
{
    const H264Picture *current_picture = h->cur_pic_ptr;
    const SPS *sps = h->ps.sps;
    const PPS *pps = h->ps.pps;
    int i, j;

    memset(pp, 0, sizeof(*pp));
    /* Configure current picture */
    fill_picture_entry(&pp->CurrPic,
                       (unsigned)current_picture->f->data[4],
                       h->picture_structure == PICT_BOTTOM_FIELD);
    /* Configure the set of references */
    pp->UsedForReferenceFlags  = 0;
    pp->NonExistingFrameFlags  = 0;
    for (i = 0, j = 0; i < FF_ARRAY_ELEMS(pp->RefFrameList); i++) {
        const H264Picture *r;
        if (j < h->short_ref_count) {
            r = h->short_ref[j++];
        } else {
            r = NULL;
            while (!r && j < h->short_ref_count + 16)
                r = h->long_ref[j++ - h->short_ref_count];
        }
        if (r) {
            fill_picture_entry(&pp->RefFrameList[i],
                               (unsigned)r->f->data[4],
                               r->long_ref != 0);

            if ((r->reference & PICT_TOP_FIELD) && r->field_poc[0] != INT_MAX)
                pp->FieldOrderCntList[i][0] = r->field_poc[0];
            if ((r->reference & PICT_BOTTOM_FIELD) && r->field_poc[1] != INT_MAX)
                pp->FieldOrderCntList[i][1] = r->field_poc[1];

            pp->FrameNumList[i] = r->long_ref ? r->pic_id : r->frame_num;
            if (r->reference & PICT_TOP_FIELD)
                pp->UsedForReferenceFlags |= 1 << (2*i + 0);
            if (r->reference & PICT_BOTTOM_FIELD)
                pp->UsedForReferenceFlags |= 1 << (2*i + 1);
        } else {
            pp->RefFrameList[i].bPicEntry = 0xff;
            pp->FieldOrderCntList[i][0]   = 0;
            pp->FieldOrderCntList[i][1]   = 0;
            pp->FrameNumList[i]           = 0;
        }
    }

    pp->wFrameWidthInMbsMinus1        = h->mb_width  - 1;
    pp->wFrameHeightInMbsMinus1       = h->mb_height - 1;
    pp->num_ref_frames                = sps->ref_frame_count;

    pp->wBitFields                    = ((h->picture_structure != PICT_FRAME) <<  0) |
                                        ((sps->mb_aff &&
                                        (h->picture_structure == PICT_FRAME)) <<  1) |
                                        (sps->residual_color_transform_flag   <<  2) |
                                        /* sp_for_switch_flag (not implemented by FFmpeg) */
                                        (0                                    <<  3) |
                                        (sps->chroma_format_idc               <<  4) |
                                        ((h->nal_ref_idc != 0)                <<  6) |
                                        (pps->constrained_intra_pred          <<  7) |
                                        (pps->weighted_pred                   <<  8) |
                                        (pps->weighted_bipred_idc             <<  9) |
                                        /* MbsConsecutiveFlag */
                                        (1                                    << 11) |
                                        (sps->frame_mbs_only_flag             << 12) |
                                        (pps->transform_8x8_mode              << 13) |
                                        ((sps->level_idc >= 31)               << 14) |
                                        /* IntraPicFlag (Modified if we detect a non
                                         * intra slice in dxva_decode_slice) */
                                        (1                                    << 15);

    pp->bit_depth_luma_minus8         = sps->bit_depth_luma - 8;
    pp->bit_depth_chroma_minus8       = sps->bit_depth_chroma - 8;
    if (ctx->workaround & FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG)
        pp->Reserved16Bits            = 0;
    else if (ctx->workaround & FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO)
        pp->Reserved16Bits            = 0x34c;
    else
        pp->Reserved16Bits            = 3; /* FIXME is there a way to detect the right mode ? */
    pp->StatusReportFeedbackNumber    = 1 + ctx->report_id++;
    pp->CurrFieldOrderCnt[0] = 0;
    if ((h->picture_structure & PICT_TOP_FIELD) &&
        current_picture->field_poc[0] != INT_MAX)
        pp->CurrFieldOrderCnt[0] = current_picture->field_poc[0];
    pp->CurrFieldOrderCnt[1] = 0;
    if ((h->picture_structure & PICT_BOTTOM_FIELD) &&
        current_picture->field_poc[1] != INT_MAX)
        pp->CurrFieldOrderCnt[1] = current_picture->field_poc[1];
    pp->pic_init_qs_minus26           = pps->init_qs - 26;
    pp->chroma_qp_index_offset        = pps->chroma_qp_index_offset[0];
    pp->second_chroma_qp_index_offset = pps->chroma_qp_index_offset[1];
    pp->ContinuationFlag              = 1;
    pp->pic_init_qp_minus26           = pps->init_qp - 26;
    pp->num_ref_idx_l0_active_minus1  = pps->ref_count[0] - 1;
    pp->num_ref_idx_l1_active_minus1  = pps->ref_count[1] - 1;
    pp->Reserved8BitsA                = 0;
    pp->frame_num                     = h->poc.frame_num;
    pp->log2_max_frame_num_minus4     = sps->log2_max_frame_num - 4;
    pp->pic_order_cnt_type            = sps->poc_type;
    if (sps->poc_type == 0)
        pp->log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_poc_lsb - 4;
    else if (sps->poc_type == 1)
        pp->delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
    pp->direct_8x8_inference_flag     = sps->direct_8x8_inference_flag;
    pp->entropy_coding_mode_flag      = pps->cabac;
    pp->pic_order_present_flag        = pps->pic_order_present;
    pp->num_slice_groups_minus1       = pps->slice_group_count - 1;
    pp->slice_group_map_type          = pps->mb_slice_group_map_type;
    pp->deblocking_filter_control_present_flag = pps->deblocking_filter_parameters_present;
    pp->redundant_pic_cnt_present_flag= pps->redundant_pic_cnt_present;
    pp->Reserved8BitsB                = 0;
    pp->slice_group_change_rate_minus1= 0;  /* XXX not implemented by FFmpeg */
    //pp->SliceGroupMap[810];               /* XXX not implemented by FFmpeg */
    pp->MinLumaBipredSize8x8Flag      = 1;  /* Improve accelerator performances */
}

static void fill_scaling_lists(dxva_context *ctx,
                               struct DXVA_H264_Picture_Context *ctx_pic,
                               const H264Context *h)
{
    const PPS *pps = h->ps.pps;
    unsigned i, j;
    DXVA_Qmatrix_H264 *qm = &ctx_pic->qm;
    memset(qm, 0, sizeof(*qm));
    if (ctx->workaround & FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG) {
        for (i = 0; i < 6; i++)
            for (j = 0; j < 16; j++)
                qm->bScalingLists4x4[i][j] = pps->scaling_matrix4[i][j];

        for (i = 0; i < 64; i++) {
            qm->bScalingLists8x8[0][i] = pps->scaling_matrix8[0][i];
            qm->bScalingLists8x8[1][i] = pps->scaling_matrix8[3][i];
        }
    } else {
        for (i = 0; i < 6; i++)
            for (j = 0; j < 16; j++)
                qm->bScalingLists4x4[i][j] = pps->scaling_matrix4[i][ff_zigzag_scan[j]];

        for (i = 0; i < 64; i++) {
            qm->bScalingLists8x8[0][i] = pps->scaling_matrix8[0][ff_zigzag_direct[i]];
            qm->bScalingLists8x8[1][i] = pps->scaling_matrix8[3][ff_zigzag_direct[i]];
        }
    }
}


static int dxva_start_frame(AVCodecContext *avctx,
                            struct DXVA_H264_Picture_Context *ctx_pic)
{
    const H264Context *h = avctx->priv_data;
    dxva_context* ctx    = (dxva_context*)avctx->dxva_context;

    assert(ctx_pic);

	memset(ctx_pic, 0, sizeof(*ctx_pic));

	/* Fill up DXVA_PicParams_H264 */
    fill_picture_parameters(h, ctx, &ctx_pic->pp);

    /* Fill up DXVA_Qmatrix_H264 */
    fill_scaling_lists(ctx, ctx_pic, h);

    return 0;
}

static int get_refpic_index(const DXVA_PicParams_H264 *pp, int surface_index)
{
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if ((pp->RefFrameList[i].bPicEntry & 0x7f) == surface_index)
          return i;
    }
    return 0x7f;
}

static void fill_slice_long(AVCodecContext *avctx, DXVA_Slice_H264_Long *slice,
                            const DXVA_PicParams_H264 *pp, unsigned position, unsigned size)
{
    const H264Context *h       = avctx->priv_data;
    const H264SliceContext *sl = &h->slice_ctx[0];
    const dxva_context* ctx    = (dxva_context*)avctx->dxva_context;
    unsigned list;

    memset(slice, 0, sizeof(*slice));
    slice->BSNALunitDataLocation = position;
    slice->SliceBytesInBuffer    = size;
    slice->wBadSliceChopping     = 0;

    slice->first_mb_in_slice     = (sl->mb_y >> FIELD_OR_MBAFF_PICTURE(h)) * h->mb_width + sl->mb_x;
    slice->NumMbsForSlice        = 0; /* XXX it is set once we have all slices */
    slice->BitOffsetToSliceData  = get_bits_count(&sl->gb) - 8;
    slice->slice_type            = ff_h264_get_slice_type(sl);
    if (sl->slice_type_fixed)
        slice->slice_type += 5;
    slice->luma_log2_weight_denom       = sl->pwt.luma_log2_weight_denom;
    slice->chroma_log2_weight_denom     = sl->pwt.chroma_log2_weight_denom;
    if (sl->list_count > 0)
        slice->num_ref_idx_l0_active_minus1 = sl->ref_count[0] - 1;
    if (sl->list_count > 1)
        slice->num_ref_idx_l1_active_minus1 = sl->ref_count[1] - 1;
    slice->slice_alpha_c0_offset_div2   = sl->slice_alpha_c0_offset / 2;
    slice->slice_beta_offset_div2       = sl->slice_beta_offset     / 2;
    slice->Reserved8Bits                = 0;

    for (list = 0; list < 2; list++) {
        unsigned i;
        for (i = 0; i < FF_ARRAY_ELEMS(slice->RefPicList[list]); i++) {
            if (list < sl->list_count && i < sl->ref_count[list]) {
                const H264Picture *r = sl->ref_list[list][i].parent;
                unsigned plane;
                unsigned index;
                if (ctx->workaround & FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO)
                    index = (unsigned)r->f->data[4];
                else
                    index = get_refpic_index(pp, (unsigned)r->f->data[4]);
                fill_picture_entry(&slice->RefPicList[list][i], index,
                                   sl->ref_list[list][i].reference == PICT_BOTTOM_FIELD);
                for (plane = 0; plane < 3; plane++) {
                    int w, o;
                    if (plane == 0 && sl->pwt.luma_weight_flag[list]) {
                        w = sl->pwt.luma_weight[i][list][0];
                        o = sl->pwt.luma_weight[i][list][1];
                    } else if (plane >= 1 && sl->pwt.chroma_weight_flag[list]) {
                        w = sl->pwt.chroma_weight[i][list][plane-1][0];
                        o = sl->pwt.chroma_weight[i][list][plane-1][1];
                    } else {
                        w = 1 << (plane == 0 ? sl->pwt.luma_log2_weight_denom :
                                               sl->pwt.chroma_log2_weight_denom);
                        o = 0;
                    }
                    slice->Weights[list][i][plane][0] = w;
                    slice->Weights[list][i][plane][1] = o;
                }
            } else {
                unsigned plane;
                slice->RefPicList[list][i].bPicEntry = 0xff;
                for (plane = 0; plane < 3; plane++) {
                    slice->Weights[list][i][plane][0] = 0;
                    slice->Weights[list][i][plane][1] = 0;
                }
            }
        }
    }
    slice->slice_qs_delta    = 0; /* XXX not implemented by FFmpeg */
    slice->slice_qp_delta    = sl->qscale - h->ps.pps->init_qp;
    slice->redundant_pic_cnt = sl->redundant_pic_count;
    if (sl->slice_type == AV_PICTURE_TYPE_B)
        slice->direct_spatial_mv_pred_flag = sl->direct_spatial_mv_pred;
    slice->cabac_init_idc = h->ps.pps->cabac ? sl->cabac_init_idc : 0;
    if (sl->deblocking_filter < 2)
        slice->disable_deblocking_filter_idc = 1 - sl->deblocking_filter;
    else
        slice->disable_deblocking_filter_idc = sl->deblocking_filter;
    slice->slice_id = h->current_slice - 1;
}

static void fill_slice_short(DXVA_Slice_H264_Short *slice,
                             unsigned position, unsigned size)
{
    memset(slice, 0, sizeof(*slice));
    slice->BSNALunitDataLocation = position;
    slice->SliceBytesInBuffer    = size;
    slice->wBadSliceChopping     = 0;
}

static int dxva_decode_slice(AVCodecContext *avctx,
                             struct DXVA_H264_Picture_Context *ctx_pic,
                             const uint8_t *buffer,
                             uint32_t size)
{
    const H264Context *h       = avctx->priv_data;
    const H264SliceContext *sl = &h->slice_ctx[0];
    const dxva_context* ctx    = (dxva_context*)avctx->dxva_context;
    unsigned position;

    if (ctx_pic->slice_count >= MAX_H264_SLICES)
        return -1;

    if (!ctx_pic->bitstream)
        ctx_pic->bitstream = buffer;
    ctx_pic->bitstream_size += size;

    position = buffer - ctx_pic->bitstream;
    if (!ctx->longslice)
        fill_slice_short(&ctx_pic->slice_short[ctx_pic->slice_count],
                         position, size);
    else
        fill_slice_long(avctx, &ctx_pic->slice_long[ctx_pic->slice_count],
                        &ctx_pic->pp, position, size);
    ctx_pic->slice_count++;

    if (sl->slice_type != AV_PICTURE_TYPE_I && sl->slice_type != AV_PICTURE_TYPE_SI)
        ctx_pic->pp.wBitFields &= ~(1 << 15); /* Set IntraPicFlag to 0 */
    return 0;
}
