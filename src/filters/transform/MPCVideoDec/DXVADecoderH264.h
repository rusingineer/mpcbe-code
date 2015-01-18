/*
 * (C) 2006-2015 see Authors.txt
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

#pragma once

#include "DXVADecoder.h"
#include <ffmpeg/libavcodec/dxva_h264.h>

class CDXVADecoderH264 : public CDXVADecoder
{
public:
	CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderH264();

	virtual void		Flush();
	virtual HRESULT		CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize = UINT_MAX);
	virtual HRESULT		DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

private:
	DXVA_H264_Context	m_DXVA_Context;

	bool				m_bUseLongSlice;

	UINT				m_nFieldNum;
	UINT				StatusReportFeedbackNumber;
	USHORT				Reserved16Bits;

	void				Init();
};
