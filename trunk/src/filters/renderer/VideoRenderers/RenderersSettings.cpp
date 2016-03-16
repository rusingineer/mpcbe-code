/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

#include "stdafx.h"
#include "RenderersSettings.h"
#include "../../../apps/mplayerc/mplayerc.h"
#include <version.h>
#include <dx/d3dx9.h>

void CRenderersSettings::SaveRenderers()
{
	AfxGetAppSettings().SaveRenderers();
}

void CRenderersSettings::CAdvRendererSettings::SetDefault()
{
	bAlterativeVSync				= false;
	iVSyncOffset					= 0;
	bVSyncAccurate					= false;
	bVSync							= false;
	bColorManagementEnable			= false;
	iColorManagementInput			= VIDEO_SYSTEM_UNKNOWN;
	iColorManagementAmbientLight	= AMBIENT_LIGHT_BRIGHT;
	iColorManagementIntent			= COLOR_RENDERING_INTENT_PERCEPTUAL;
	bDisableDesktopComposition		= false;
	bFlushGPUBeforeVSync			= false;
	bFlushGPUAfterPresent			= false;
	bFlushGPUWait					= false;
	bEVRFrameTimeCorrection			= false;
	iEVROutputRange					= 0;

	iSynchronizeMode				= SYNCHRONIZE_NEAREST;
	iLineDelta						= 0;
	iColumnDelta					= 0;
	dCycleDelta						= 0.0012;
	dTargetSyncOffset				= 12.0;
	dControlLimit					= 2.0;
}

/////////////////////////////////////////////////////////////////////////////
// CRenderersData construction

CRenderersData::CRenderersData()
{
	m_fTearingTest  = false;
	m_fDisplayStats = false;
	m_bResetStats   = false;
	m_hD3DX9Dll     = NULL;

	// Don't disable hardware features before initializing a renderer
	m_bFP16Support  = true;
	m_b10bitSupport = true;
}


HINSTANCE GetD3X9Dll()
{
#if D3DX_SDK_VERSION < 43
#error DirectX SDK June 2010 (v43) or newer is required to build MPC-BE
#endif
	static HINSTANCE s_hD3DX9Dll = NULL;

	if (s_hD3DX9Dll == NULL) {
		// load latest compatible version of the DLL that is available
		s_hD3DX9Dll = LoadLibrary(L"d3dx9_43.dll");
	}

	return s_hD3DX9Dll;
}