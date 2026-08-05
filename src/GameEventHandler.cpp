#include "GameEventHandler.h"
#include "Hooks.h"
#include <detours/detours.h>
#include <DirectXMath.h>
#pragma warning(disable : 4189)
#define M_PI 3.1415926535897932384626433832795f
static bool patched = false;
namespace plugin {
    auto orig_SetFrameBufferMatricesHook = (void (*)(void *t, uint64_t arg2)) nullptr;
    DirectX::XMMATRIX viewcache;
    DirectX::XMMATRIX projcache;
    bool copy_to_cache = true;
    bool restore_from_cache = false;
    bool replace_projection_matrix = true;
    bool renderLeft = true;
    float eyeSeparation = 0.05f * 70.0f;
    float converge = 0.005f;
    void SetFrameBufferMatricesHook(void *t, uint64_t arg2) {
        DirectX::XMMATRIX *view = (DirectX::XMMATRIX *) REL::RelocationID(524875, 388922).address();
        DirectX::XMMATRIX *proj = &view[1];
        DirectX::XMMATRIX *viewproj = &view[2];
        DirectX::XMMATRIX *viewprojunj = &view[3];
        DirectX::XMMATRIX *prevviewprojunj = &view[4];
        DirectX::XMMATRIX *projunj = &view[5];
        DirectX::XMMATRIX *projunjinv = &view[6];
        DirectX::XMMATRIX *viewinv = &view[7];
        DirectX::XMMATRIX *projinv = &view[8];
        DirectX::XMMATRIX *viewprojinv = &view[9];
        //DirectX::XMMATRIX *viewproj = (DirectX::XMMATRIX *) (REL::RelocationID(524884, 388931).address());
        float oldmatrices[16 * 10];
        memcpy(oldmatrices, view, 640);
        if (copy_to_cache) {
            viewcache = *view;
            projcache = *proj;
        }
        if (restore_from_cache) {
            *view = viewcache;
            *proj = projcache;
            *viewproj = XMMatrixMultiply(viewcache, projcache);
        }

        if (!RE::BSGraphics::Renderer::GetSingleton()) {
            return;
        }

        auto size = RE::BSGraphics::Renderer::GetSingleton()->GetScreenSize();

        float aspectRatio = ((float) size.width) / ((float) size.height);
        if (replace_projection_matrix == true) {
            DirectX::XMMATRIX newview;
            DirectX::XMMATRIX newproj;
            DirectX::XMMATRIX newviewproj;
            {
                DirectX::XMMATRIX *view_in = view;
                DirectX::XMMATRIX *proj_in = proj;
                auto original_vfov = atanf(1.0f / proj_in->r[1].m128_f32[1]) * 2.0f;
                auto original_nZ = proj_in->r[3].m128_f32[2] / (-proj_in->r[2].m128_f32[2]);
                auto original_fZ = ((proj_in->r[2].m128_f32[2] * original_nZ) / (proj_in->r[2].m128_f32[2] - 1.0f));
                float nZ = original_nZ;
                float fZ = original_fZ;
                float fov = original_vfov * aspectRatio;
                float vFov = fov / aspectRatio;
                float viewHeight = 2.0f * nZ * tanf(vFov / 2.0f);
                float viewWidth = viewHeight * aspectRatio;
                float shift = renderLeft ? (eyeSeparation / 2.0f) : (-eyeSeparation / 2.0f);
                float frustum_shift = renderLeft ? (converge * viewWidth) : -(converge * viewWidth);
                float left_left = -viewWidth / 2.0f - (frustum_shift);
                float left_right = viewWidth / 2.0f - (frustum_shift);
                float left_bottom = -viewHeight / 2.0f;
                float left_top = viewHeight / 2.0f;
                newview = XMMatrixMultiply((*view_in), DirectX::XMMatrixTranslation(-shift, 0.0, 0.0));
                newproj = DirectX::XMMatrixPerspectiveOffCenterLH(left_left, left_right, left_bottom, left_top, nZ, fZ);
                newviewproj = XMMatrixMultiply(newview, newproj);
                *viewprojunj = newviewproj;
                *prevviewprojunj = newviewproj;
                *projunj = newproj;
                *projunjinv = newproj;
                *projunjinv = DirectX::XMMatrixTranspose(*projunjinv);
                *viewinv = newview;
                *viewinv = DirectX::XMMatrixTranspose(*viewinv);
                *projinv = newproj;
                *projinv = DirectX::XMMatrixTranspose(*projinv);
                *viewprojinv = newviewproj;
                *viewprojinv = DirectX::XMMatrixTranspose(*viewprojinv);
                *view = newview;
                *proj = newproj;
                *viewproj = newviewproj;
            }
            orig_SetFrameBufferMatricesHook(t, arg2);
            memcpy(view,oldmatrices, 640);
        } else {
            return orig_SetFrameBufferMatricesHook(t, arg2);
        }
    }
    auto orig_SceneUpdateD = (void (*)(uint32_t a)) nullptr;
    void SceneUpdateD(uint32_t a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                return;
            } else {
                orig_SceneUpdateD(a);
            }
        }
    }
    auto orig_SceneUpdateC = (void (*)(uint32_t a)) nullptr;
    void SceneUpdateC(uint32_t a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                return;
            } else {
                orig_SceneUpdateC(a);
            }
        }
    }
    auto orig_SceneUpdateB = (void (*)(void *a)) nullptr;
    void SceneUpdateB(void *a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                orig_SceneUpdateB(a);
            } else {
                orig_SceneUpdateB(a);
            }
        } else {
            orig_SceneUpdateB(a);
        }
    }
    auto orig_SceneUpdate = (void (*)(void *a)) nullptr;
    void SceneUpdate(void *a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                orig_SceneUpdate(a);
                (*(uint32_t *) REL::RelocationID(525008, 411489).address()) -= 1;
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();

                return;
            } else {
                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                orig_SceneUpdate(a);
                (*(uint32_t *) REL::RelocationID(525008, 411489).address()) -= 1;
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();

                return;
            }
        } else {
            orig_SceneUpdate(a);
        }
    }
    static bool DoFrameCounterBugFix = false;
    auto FrameCounterBug = (void (*)(RE::NiAVObject **)) nullptr;
    
    void FrameCounterBugFix(RE::NiAVObject** obj) {
        if (obj[2]->GetRTTI()->IsKindOf((RE::NiRTTI *) RE::NiRTTI_BSDynamicTriShape.address())) {
            RE::BSDynamicTriShape *shape = (RE::BSDynamicTriShape *)obj[2];
            if (shape) {
                if (DoFrameCounterBugFix) {
                    if (shape->GetDynamicTrishapeRuntimeData().frameCount != (*(uint32_t*)REL::RelocationID(525008, 411489).address())) {
                        shape->GetDynamicTrishapeRuntimeData().frameCount = (*(uint32_t *) REL::RelocationID(525008, 411489).address());
                    }
                    if (!shape->GetGeometryRuntimeData().rendererData) {
                        logger::info("rendererData was missing");
                        return;
                    }
                }
            }
        }
        FrameCounterBug(obj);
    }
    auto orig_DrawCallHook = (void (*)(void *t, float)) nullptr;
    void DrawCallHook(void *t, float mode) {
        if (replace_projection_matrix == true) {
            if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
                float *WorldTimeFrame = (float *) REL::RelocationID(523660, 410199).address();
                float *RealTimeFrame = (float *) REL::RelocationID(523661, 410200).address();

                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                renderLeft = ((*(uint32_t *) REL::RelocationID(525008, 411489).address()) & 1) == 0;
                orig_DrawCallHook(t, mode);

                renderLeft = !renderLeft;
                DoFrameCounterBugFix = true;
                orig_DrawCallHook(t, mode);
                (*(uint32_t *) REL::RelocationID(525008, 411489).address()) += 2;
                DoFrameCounterBugFix = false;
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();
            }
        } else {
            orig_DrawCallHook(t, mode);
        }
    }
    auto orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) nullptr;

    void GameEventHandler::onLoad() {
        logger::info("onLoad()");
        Hooks::install();
        if (patched == false) {
            auto version = REL::Module::get().version();
            if (version == REL::Version(1, 6, 1170, 0)) {
                FrameCounterBug = (void (*)(RE::NiAVObject **)) REL::RelocationID(100847, 107637).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) FrameCounterBug,FrameCounterBugFix);
                DetourTransactionCommit();
                orig_SetFrameBufferMatricesHook = (void (*)(void *t, uint64_t arg2)) REL::RelocationID(0, 77258).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_SetFrameBufferMatricesHook, SetFrameBufferMatricesHook);
                DetourTransactionCommit();
                auto &trampoline = SKSE::GetTrampoline();
                SKSE::AllocTrampoline(14);
                orig_DrawCallHook =
                    (void (*)(void *t, float delta)) trampoline.write_call<5>(REL::RelocationID(0, 36564).address() + 0xa97, DrawCallHook);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdate = (void (*)(void *a)) trampoline.write_call<5>(REL::RelocationID(0, 36555).address() + 0x601, SceneUpdate);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdateB =
                    (void (*)(void *a)) trampoline.write_call<5>(REL::RelocationID(0, 36555).address() + 0x5f0, SceneUpdateB);
                

                patched = true;
            } else if (version == REL::Version(1, 5, 97, 0)) {
                FrameCounterBug = (void (*)(RE::NiAVObject **)) REL::RelocationID(100847,107637).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) FrameCounterBug,FrameCounterBugFix);
                DetourTransactionCommit();
                orig_SetFrameBufferMatricesHook = (void (*)(void *t, uint64_t arg2)) REL::RelocationID(75472,0).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_SetFrameBufferMatricesHook, SetFrameBufferMatricesHook);
                DetourTransactionCommit();
                auto &trampoline = SKSE::GetTrampoline();
                SKSE::AllocTrampoline(14);
                orig_DrawCallHook =
                    (void (*)(void *t, float delta)) trampoline.write_call<5>(REL::RelocationID(35565, 0).address() + 0x5d2, DrawCallHook);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdate =
                    (void (*)(void *a)) trampoline.write_call<5>(REL::RelocationID(35556, 0).address() + 0x596, SceneUpdate);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdateB =
                    (void (*)(void *a)) trampoline.write_call<5>(REL::RelocationID(35556, 0).address() + 0x585, SceneUpdateB);

                patched = true;
            }
        }
    }

    void GameEventHandler::onPostLoad() {
        logger::info("onPostLoad()");
    }

    void GameEventHandler::onPostPostLoad() {
        logger::info("onPostPostLoad()");
    }

    void GameEventHandler::onInputLoaded() {
        logger::info("onInputLoaded()");
    }
    bool SetSeparation(RE::StaticFunctionTag *, float value) {
        eyeSeparation = value;
        return true;
    }
    bool SetConverge(RE::StaticFunctionTag *, float value) {
        converge = value;
        return true;
    }
    void GameEventHandler::onDataLoaded() {
        RE::SkyrimVM::GetSingleton()->impl->RegisterFunction("SetSeparation", "AEVRSSE", SetSeparation, false);
        RE::SkyrimVM::GetSingleton()->impl->RegisterFunction("SetConverge", "AEVRSSE", SetConverge, false);
        auto light_array = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectLIGH>();
        for (auto *light: light_array) {
            light->data.flags.reset(RE::TES_LIGHT_FLAGS::kHemiShadow);
            light->data.flags.reset(RE::TES_LIGHT_FLAGS::kOmniShadow);
            light->data.flags.reset(RE::TES_LIGHT_FLAGS::kSpotShadow);
        }
        logger::info("onDataLoaded()");
    }

    void GameEventHandler::onNewGame() {
        logger::info("onNewGame()");
    }

    void GameEventHandler::onPreLoadGame() {
        logger::info("onPreLoadGame()");
    }

    void GameEventHandler::onPostLoadGame() {
        logger::info("onPostLoadGame()");
    }

    void GameEventHandler::onSaveGame() {
        logger::info("onSaveGame()");
    }

    void GameEventHandler::onDeleteGame() {
        logger::info("onDeleteGame()");
    }
}  // namespace plugin