#pragma once

class Settings
{
public:
	struct LockData
	{
		std::string chestModel{ "Interface/Lockpicking/LockPickShiv01.nif" };
		std::string doorModel{ "Interface/Lockpicking/LockPickShiv01.nif" };
	};

	struct SoundData
	{
		std::string UILockpickingCylinderSqueakA{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderSqueakB{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderStop{ "UILockpickingCylinderStop" };
		std::string UILockpickingCylinderTurn{ "UILockpickingCylinderTurn" };
		std::string UILockpickingPickMovement{ "UILockpickingPickMovement" };
		std::string UILockpickingUnlock{ "UILockpickingUnlock" };
	};

	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	bool Load()
	{
		std::vector<std::string> configs;

		auto constexpr folder = R"(Data\)";
		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
				if (const auto path = entry.path().string(); path.find("_LID") != std::string::npos) {
					configs.push_back(path);
				}
			}
		}

		if (configs.empty()) {
			logger::warn("	No .ini files with _LID suffix were found within the Data folder, aborting...");
			return false;
		}

		logger::info("	{} matching inis found", configs.size());

		for (auto& path : configs) {
			logger::info("		INI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				logger::error("	couldn't read INI");
				continue;
			}

			std::list<CSimpleIniA::Entry> list;
			ini.GetAllSections(list);
			for (auto& [section, comment, order] : list) {
				LockData lock{};
				SoundData sound{};

				detail::get_value(ini, lock.chestModel, section, "Chest");
				detail::get_value(ini, lock.doorModel, section, "Door");

				detail::get_value(ini, sound.UILockpickingCylinderSqueakA, section, "CylinderSqueakA");
				detail::get_value(ini, sound.UILockpickingCylinderSqueakB, section, "CylinderSqueakB");
				detail::get_value(ini, sound.UILockpickingCylinderStop, section, "CylinderStop");
				detail::get_value(ini, sound.UILockpickingCylinderTurn, section, "CylinderTurn");
				detail::get_value(ini, sound.UILockpickingPickMovement, section, "PickMovement");
				detail::get_value(ini, sound.UILockpickingUnlock, section, "LockpickingUnlock");

				lockDataMap[section] = lock;
				soundDataMap[section] = sound;
			}
		}

		return !lockDataMap.empty();
	}

	std::string GetLockModel(const char* a_fallbackPath)
	{
		//reset
		lockType = std::nullopt;
		std::optional<LockData> lockData = std::nullopt;

		auto ref = RE::LockpickingMenu::GetTargetReference();
		if (ref) {
			auto base = ref ? ref->GetBaseObject() : nullptr;
			auto model = base ? base->As<RE::TESModel>() : nullptr;

			if (base && model) {
				auto it = std::find_if(lockDataMap.begin(), lockDataMap.end(), [&](const auto& data) {
					return string::icontains(model->GetModel(), data.first);
				});
				if (it != lockDataMap.end()) {
					lockType = it->first;
					lockData = it->second;
				}
				if (!lockData) {
					auto modelSwap = model->GetAsModelTextureSwap();
					if (modelSwap && modelSwap->alternateTextures) {
						std::span span(modelSwap->alternateTextures, modelSwap->numAlternateTextures);
						for (const auto& texture : span) {
							const auto txst = texture.textureSet;
							if (txst && string::icontains(txst->textures[RE::BSTextureSet::Texture::kDiffuse].textureName, "snow"sv)) {
								if (it = lockDataMap.find("IceCastle"); it != lockDataMap.end()) {
									lockType = it->first;
									lockData = it->second;
								}
								break;
							}
						}
					}
				}
				if (lockData) {
					return base->Is(RE::FormType::Door) ? lockData->doorModel : lockData->chestModel;
				}
			}
		}
	
		return a_fallbackPath;
	}

	std::optional<SoundData> GetSoundData()
	{
		if (lockType) {
			return soundDataMap[*lockType];
		}
		return std::nullopt;
	}

private:
	struct detail
	{
		static void get_value(CSimpleIniA& a_ini, std::string& a_value, const char* a_section, const char* a_key)
		{
			a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
		};
	};

	std::map<std::string, LockData> lockDataMap;
	std::map<std::string, SoundData> soundDataMap;

	std::optional<std::string> lockType;
};