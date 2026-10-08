#include "Hooks.h"
#include "Manager.h"

namespace Hook
{
	namespace
	{
		// True if a_src is "E8 rel32" and the callee is an empty function (ret / ret 0).
		bool IsCallToNullsub(std::uintptr_t a_src)
		{
			const auto* code = reinterpret_cast<const std::uint8_t*>(a_src);
			if (code[0] != 0xE8) {
				return false;
			}
			const auto rel = *reinterpret_cast<const std::int32_t*>(code + 1);
			const auto* dst = reinterpret_cast<const std::uint8_t*>(a_src + 5 + rel);
			return dst[0] == 0xC3 || (dst[0] == 0xC2 && dst[1] == 0x00 && dst[2] == 0x00);
		}

		// Skyrim 1.7.x (SKSE 2.3.x) moved code around. Before patching, make sure the
		// target really is a 5-byte relative CALL (opcode E8). If it is not, the hook is
		// skipped and logged instead of corrupting the game code and crashing.
		template <class T>
		bool SafeWriteThunkCall(std::uintptr_t a_src, std::string_view a_name)
		{
			const auto opcode = *reinterpret_cast<const std::uint8_t*>(a_src);
			if (opcode != 0xE8) {
				SKSE::log::critical(
					"{}: expected a CALL (E8) at {:X} (game {}) but found {:02X}. Hook skipped, "
					"this feature will be inactive on this game version.",
					a_name, a_src - REL::Module::get().base(), REL::Module::get().version().string(), opcode);
				return false;
			}
			stl::write_thunk_call<T>(a_src);
			SKSE::log::info("{}: hooked at {:X}", a_name, a_src - REL::Module::get().base());
			return true;
		}
	}

	struct MainUpdate
	{

		static void thunk()
		{
			func(); // Run original function

			static auto lastCallTime = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();

			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallTime).count() >= 500)
			{
				lastCallTime = now;

				const auto singleton = Manager::GetSingleton();
				singleton->toggleEffectWeather();
				singleton->toggleEffectTime();
			}

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			const auto version = REL::Module::get().version();

			if (REL::Module::IsAE() && version >= SKSE::RUNTIME_SSE_1_7_99) {
				// Skyrim 1.7.x: Main::Update was reorganised. The per-frame nullsub call that
				// this hook relies on sits at +0xC3D on 1.7.104 (verified on the 1.7.104 executable)
				// and at +0xC38 on 1.7.99. Pick the candidate that really is a CALL to a nullsub.
				REL::Relocation<std::uintptr_t> mainUpdate{ RELOCATION_ID(35565, 36564) };
				for (const std::uintptr_t offset : { 0xC3Du, 0xC38u }) {
					const auto src = mainUpdate.address() + offset;
					if (IsCallToNullsub(src)) {
						SafeWriteThunkCall<MainUpdate>(src, "MainUpdate"sv);
						return;
					}
				}
				SKSE::log::critical("MainUpdate: no suitable call site found on game {}. Hook skipped, "
									"time/weather toggles will be inactive.",
					version.string());
				return;
			}

			const bool marketplace = version >= SKSE::RUNTIME_SSE_1_6_1130;
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(35565, 36564), REL::Relocate(0x748, (marketplace ? 0xC2b : 0xC26), 0x7EE) };
			SafeWriteThunkCall<MainUpdate>(target1.address(), "MainUpdate"sv);
		}
	};

	struct InteriorChange
	{
		static void thunk(bool a_playerIsInInterior) // Runs on cell load
		{
			func(a_playerIsInInterior);

			Manager::GetSingleton()->toggleEffectInterior(a_playerIsInInterior);

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(13171, 13316), REL::Relocate(0x2E6, 0x46D)),
				std::make_pair(RELOCATION_ID(13172, 13317), REL::Relocate(0x2A, 0x1E)),
			};

			for (auto& [id, offset] : targets)
			{
				REL::Relocation<std::uintptr_t> target(id, offset);
				SafeWriteThunkCall<InteriorChange>(target.address(), "InteriorChange"sv);
			}

		}
	};



	void Install()
	{
		MainUpdate::Install();
		InteriorChange::Install();

	}
}