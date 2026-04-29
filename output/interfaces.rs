// Generated using https://github.com/a2x/cs2-dumper
// 2026-04-29 11:37:06.934308600 UTC

#![allow(non_upper_case_globals, unused)]

pub mod cs2_dumper {
    pub mod interfaces {
        // Module: animationsystem.dll
        pub mod animationsystem_dll {
            pub const AnimationSystemUtils_001: usize = 0x812190;
            pub const AnimationSystem_001: usize = 0x80A0B0;
        }
        // Module: engine2.dll
        pub mod engine2_dll {
            pub const BenchmarkService001: usize = 0x612950;
            pub const BugService001: usize = 0x8CA900;
            pub const ClientServerEngineLoopService_001: usize = 0x90C000;
            pub const ClientServerSharedHandleSystem001: usize = 0x90B5B0;
            pub const EngineGameUI001: usize = 0x6102A0;
            pub const EngineServiceMgr001: usize = 0x90B8E0;
            pub const GameEventSystemClientV001: usize = 0x90BBC0;
            pub const GameEventSystemServerV001: usize = 0x90BCF0;
            pub const GameResourceServiceClientV001: usize = 0x612A50;
            pub const GameResourceServiceServerV001: usize = 0x612AB0;
            pub const GameUIService_001: usize = 0x8CAD30;
            pub const HostStateMgr001: usize = 0x613290;
            pub const INETSUPPORT_001: usize = 0x60C040;
            pub const InputService_001: usize = 0x8CB020;
            pub const KeyValueCache001: usize = 0x613340;
            pub const MapListService_001: usize = 0x909E90;
            pub const NetworkClientService_001: usize = 0x90A020;
            pub const NetworkP2PService_001: usize = 0x90A360;
            pub const NetworkServerService_001: usize = 0x90A510;
            pub const NetworkService_001: usize = 0x612C20;
            pub const RenderService_001: usize = 0x90A780;
            pub const ScreenshotService001: usize = 0x90AA40;
            pub const SimpleEngineLoopService_001: usize = 0x6133A0;
            pub const SoundService_001: usize = 0x612C60;
            pub const Source2EngineToClient001: usize = 0x60FBA0;
            pub const Source2EngineToClientStringTable001: usize = 0x60FC00;
            pub const Source2EngineToServer001: usize = 0x60FC78;
            pub const Source2EngineToServerStringTable001: usize = 0x60FCA0;
            pub const SplitScreenService_001: usize = 0x612F40;
            pub const StatsService_001: usize = 0x90AE00;
            pub const ToolService_001: usize = 0x613100;
            pub const VENGINE_GAMEUIFUNCS_VERSION005: usize = 0x610330;
            pub const VProfService_001: usize = 0x613140;
        }
        // Module: filesystem_stdio.dll
        pub mod filesystem_stdio_dll {
            pub const VAsyncFileSystem2_001: usize = 0x2159E0;
            pub const VFileSystem017: usize = 0x2157A0;
        }
        // Module: imemanager.dll
        pub mod imemanager_dll {
            pub const IMEManager001: usize = 0x36B20;
        }
        // Module: inputsystem.dll
        pub mod inputsystem_dll {
            pub const InputStackSystemVersion001: usize = 0x40E30;
            pub const InputSystemVersion001: usize = 0x42B50;
        }
        // Module: localize.dll
        pub mod localize_dll {
            pub const Localize_001: usize = 0x62180;
        }
        // Module: materialsystem2.dll
        pub mod materialsystem2_dll {
            pub const FontManager_001: usize = 0x15DE60;
            pub const MaterialUtils_001: usize = 0x145D40;
            pub const PostProcessingSystem_001: usize = 0x145C50;
            pub const TextLayout_001: usize = 0x145CD0;
            pub const VMaterialSystem2_001: usize = 0x15D750;
        }
        // Module: meshsystem.dll
        pub mod meshsystem_dll {
            pub const MeshSystem001: usize = 0x150C20;
        }
        // Module: networksystem.dll
        pub mod networksystem_dll {
            pub const FlattenedSerializersVersion001: usize = 0x26D700;
            pub const NetworkMessagesVersion001: usize = 0x2959D0;
            pub const NetworkSystemVersion001: usize = 0x286E50;
            pub const SerializedEntitiesVersion001: usize = 0x286F40;
        }
        // Module: particles.dll
        pub mod particles_dll {
            pub const ParticleSystemMgr003: usize = 0x54A3C0;
        }
        // Module: pulse_system.dll
        pub mod pulse_system_dll {
            pub const IPulseSystem_001: usize = 0x1F36A0;
        }
        // Module: rendersystemdx11.dll
        pub mod rendersystemdx11_dll {
            pub const RenderDeviceMgr001: usize = 0x42B530;
            pub const RenderUtils_001: usize = 0x42BE28;
            pub const VRenderDeviceMgrBackdoor001: usize = 0x42B5D0;
        }
        // Module: resourcesystem.dll
        pub mod resourcesystem_dll {
            pub const ResourceSystem013: usize = 0x831E0;
        }
        // Module: scenesystem.dll
        pub mod scenesystem_dll {
            pub const RenderingPipelines_001: usize = 0x663B80;
            pub const SceneSystem_002: usize = 0x8DB470;
            pub const SceneUtils_001: usize = 0x664A90;
        }
        // Module: schemasystem.dll
        pub mod schemasystem_dll {
            pub const SchemaSystem_001: usize = 0x76800;
        }
        // Module: soundsystem.dll
        pub mod soundsystem_dll {
            pub const SoundOpSystem001: usize = 0x5129C0;
            pub const SoundOpSystemEdit001: usize = 0x512880;
            pub const SoundSystem001: usize = 0x512360;
            pub const VMixEditTool001: usize = 0x59487BF;
        }
        // Module: steamaudio.dll
        pub mod steamaudio_dll {
            pub const SteamAudio001: usize = 0x25E620;
        }
        // Module: steamclient64.dll
        pub mod steamclient64_dll {
            pub const IVALIDATE001: usize = 0x16998B8;
            pub const SteamClient006: usize = 0x1696D50;
            pub const SteamClient007: usize = 0x1696D58;
            pub const SteamClient008: usize = 0x1696D60;
            pub const SteamClient009: usize = 0x1696D68;
            pub const SteamClient010: usize = 0x1696D70;
            pub const SteamClient011: usize = 0x1696D78;
            pub const SteamClient012: usize = 0x1696D80;
            pub const SteamClient013: usize = 0x1696D88;
            pub const SteamClient014: usize = 0x1696D90;
            pub const SteamClient015: usize = 0x1696D98;
            pub const SteamClient016: usize = 0x1696DA0;
            pub const SteamClient017: usize = 0x1696DA8;
            pub const SteamClient018: usize = 0x1696DB0;
            pub const SteamClient019: usize = 0x1696DB8;
            pub const SteamClient020: usize = 0x1696DC0;
            pub const SteamClient021: usize = 0x1696DC8;
            pub const SteamClient022: usize = 0x1696DD0;
            pub const SteamClient023: usize = 0x1696DD8;
            pub const p2pvoice002: usize = 0x14E5DEF;
            pub const p2pvoicesingleton002: usize = 0x16720F0;
        }
        // Module: tier0.dll
        pub mod tier0_dll {
            pub const TestScriptMgr001: usize = 0x39E6F0;
            pub const VEngineCvar007: usize = 0x3A93B0;
            pub const VProcessUtils002: usize = 0x39E690;
            pub const VStringTokenSystem001: usize = 0x3D00B0;
        }
        // Module: vphysics2.dll
        pub mod vphysics2_dll {
            pub const VPhysics2_Interface_001: usize = 0x40DDA0;
        }
        // Module: vscript.dll
        pub mod vscript_dll {
            pub const VScriptManager010: usize = 0x13B410;
        }
        // Module: vstdlib_s64.dll
        pub mod vstdlib_s64_dll {
            pub const IVALIDATE001: usize = 0x6F990;
            pub const VEngineCvar002: usize = 0x6E070;
        }
        // Module: worldrenderer.dll
        pub mod worldrenderer_dll {
            pub const WorldRendererMgr001: usize = 0x225C40;
        }
    }
}
