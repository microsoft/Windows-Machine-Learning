#include "pch.h"
#include "DX11VideoRenderer.h"
#include "Common.h"
#include "Activate.h"
#include "ClassFactory.h"
#include "MediaSink.h"

volatile long DX11VideoRenderer::CBase::s_lObjectCount = 0;
volatile long DX11VideoRenderer::CClassFactory::s_lLockCount = 0;

STDAPI CreateDX11VideoRenderer(REFIID riid, void** ppvObject)
{
    return DX11VideoRenderer::CMediaSink::CreateInstance(riid, ppvObject);
}

STDAPI CreateDX11VideoRendererActivate(HWND hwnd, IMFActivate** ppActivate)
{
    return DX11VideoRenderer::CActivate::CreateInstance(hwnd, ppActivate);
}