#include "ModSettings.hpp"
#include "ModRuntimeSettingsVar.hpp"
#include "stdafx.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVar.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarBool.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarFloat.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarInt.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarListInt.hpp>
#include "Utils.hpp"
#include <iostream>

const std::filesystem::path configPath =
    Utils::GetRootDir() / "red4ext" / "plugins" / "mod_settings" / "user.ini";

RED4ext::TTypedClass<ModSettings> modSettings("ModSettings");

RED4ext::CClass *ModSettings::GetNativeType() { return &modSettings; }

RED4ext::Handle<ModSettings> handle;

ModSettings *ModSettings::GetInstance() {
  if (!handle.instance) {
    //spdlog::info("[RED4ext] New ModSettings Instance");
    auto instance = reinterpret_cast<ModSettings *>(modSettings.AllocInstance());
    instance->listeners =
        RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>>(new RED4ext::Memory::DefaultAllocator());
    instance->variables = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator());
    instance->variablesByClass = RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>>(
        new RED4ext::Memory::DefaultAllocator);
    instance->variablesByMod = RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>>(
        new RED4ext::Memory::DefaultAllocator);
    instance->categoriesByMod =
        RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>>(new RED4ext::Memory::DefaultAllocator);
    handle = RED4ext::Handle<ModSettings>(instance);
    handle.refCount->IncRef();
  }

  return (ModSettings *)handle.instance;
}

void ModSettings::AddVariable(ModSettingsVariable *variable) {
  auto self = ModSettings::GetInstance();
  
  std::shared_lock<RED4ext::SharedMutex> _(self->variables_lock);
  variable->listeners = RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>>(new RED4ext::Memory::DefaultAllocator());

  self->variables.EmplaceBack(variable);

  auto modVars = self->variablesByMod.Get(variable->mod);
  if (modVars) {
    modVars->EmplaceBack(variable);
  } else {
    auto ra = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator);
    ra.EmplaceBack(variable);
    self->variablesByMod.Insert(variable->mod, ra);
  }

  auto classVars = self->variablesByClass.Get(variable->className);
  if (classVars) {
    classVars->EmplaceBack(variable);
  } else {
    auto ra = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator);
    ra.EmplaceBack(variable);
    self->variablesByClass.Insert(variable->className, ra);
  }

  if (variable->category != "None") {
    auto modCategories = self->categoriesByMod.Get(variable->mod);
    if (modCategories) {
      auto found = false;
      for (const auto &category : *modCategories) {
        found |= (variable->category == category);
      }
      if (!found) {
        modCategories->EmplaceBack(variable->category);
      }
    } else {
      auto ra = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator);
      ra.EmplaceBack(variable->category);
      self->categoriesByMod.Insert(variable->mod, ra);
    }
  }
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<ModSettings> *aOut, int64_t a4) {
  aFrame->code++;

  auto h = ModSettings::GetInstance();

  if (aOut) {
    h->ref.refCount->IncRef();
    *aOut = RED4ext::Handle<ModSettings>(h);
  }
}

void GetModsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::DynArray<RED4ext::CName> *aOut, int64_t a4) {
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    *aOut = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator);

    auto size = h->variablesByMod.size;
    for (uint32_t index = 0; index != size; ++index) {
      aOut->EmplaceBack(h->variablesByMod.nodeList.nodes[index].key);
    }
  }
}

void GetCategoriesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                          RED4ext::DynArray<RED4ext::CName> *aOut, int64_t a4) {
  RED4ext::CName mod;
  RED4ext::GetParameter(aFrame, &mod);
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    auto categories = h->categoriesByMod.Get(mod);
    *aOut = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator());
    if (categories) {
      for (const auto &cat : *categories) {
        aOut->EmplaceBack(cat);
      }
    }
  }
}

void GetVarsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::DynArray<RED4ext::Handle<RED4ext::user::SettingsVar>> *aOut, int64_t a4) {
  RED4ext::CName mod;
  RED4ext::CName category;
  RED4ext::GetParameter(aFrame, &mod);
  RED4ext::GetParameter(aFrame, &category);
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    //*aOut = RED4ext::DynArray<RED4ext::Handle<RED4ext::user::SettingsVar>>(new RED4ext::Memory::DefaultAllocator);
    auto modVars = h->variablesByMod.Get(mod);
    for (const auto &variable : *modVars) {
      if (variable->category != category)
        continue;
      RED4ext::user::SettingsVar *configVar = NULL;
      switch (variable->settingsVar->type) {
      case RED4ext::user::EConfigVarType::Bool:
        configVar = (RED4ext::user::SettingsVarBool *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarBool")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::Float:
        configVar = (RED4ext::user::SettingsVarFloat *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarFloat")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::Int:
        configVar = (RED4ext::user::SettingsVarInt *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarInt")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::IntList:
        configVar = (RED4ext::user::SettingsVarListInt *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarListInt")
                        ->AllocInstance();
        break;
      }
      if (configVar) {
        configVar->runtimeVar = variable->settingsVar;
        auto h = RED4ext::Handle<RED4ext::user::SettingsVar>(configVar);
        h.refCount->IncRef();
        aOut->EmplaceBack(h);
      }
    }
  }
}

void ModSettings::WriteToFile() {
  auto self = ModSettings::GetInstance();
  std::ofstream configFile(configPath);
  if (configFile.is_open()) {
    for (const auto &node : self->variablesByClass) {
      configFile << "[" << node.key.ToString() << "]\n";
      for (const auto &variable : node.value) {
        variable->settingsVar->ApplyChange();
        if (variable->settingsVar->WasModifiedSinceLastSave()) {
          variable->UpdateValues();
          variable->settingsVar->ChangeWasWritten();
        }
        const char * value;
        auto str = RED4ext::CString::CString(new RED4ext::Memory::DefaultAllocator());
        RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(variable->settingsVar->GetValuePtr(), str);
        value = str.c_str();

        configFile << variable->settingsVar->name.ToString() << " = " << value << "\n";
      }
      configFile << "\n";
    }
    configFile.close();
    //std::string configPathStr(configPath.string());
    //spdlog::info("User settings written to: {}", configPath.string());
  } else {
    //spdlog::error("Could not write to file: {}", configPath.string());
  }
}

bool ModSettings::GetSettingString(RED4ext::CName className, RED4ext::CName propertyName, RED4ext::CString *value) {
  auto self = ModSettings::GetInstance();
  std::string defaultValue = "";
  auto valueStr = self->reader.Get(className.ToString(), propertyName.ToString(), defaultValue);
  if (valueStr != defaultValue) {
    *value = RED4ext::CString(valueStr.c_str());
    return true;
  } else {
    return false;
  }
}

void ModSettings::ReadFromFile() {
  auto self = ModSettings::GetInstance();
  self->reader = INIReader::INIReader(configPath.string());

  if (self->reader.ParseError() != 0) {
    return;
  }
}

void AcceptChangesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  aFrame->code++;
  ModSettings::WriteToFile();
  auto ms = ModSettings::GetInstance();
  ms->changeMade = false;
  ms->NotifyListeners();
}

void RestoreDefaultsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  RED4ext::CName mod;
  RED4ext::GetParameter(aFrame, &mod);
  aFrame->code++;
  auto ms = ModSettings::GetInstance();
  auto vars = ms->variablesByMod.Get(mod);
  if (vars) {
    for (const auto &variable : *vars) {
      variable->settingsVar->RestoreDefault(0);
    }
    ms->changeMade = false;
    for (const auto &variable : *vars) {
      ms->changeMade |= variable->settingsVar->WasModifiedSinceLastSave();
    }
    ms->NotifyListeners();
  }
}

void RejectChangesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  aFrame->code++;
  auto ms = ModSettings::GetInstance();

  for (const auto &variable : ms->variables) {
    if (variable->settingsVar->HasChange()) {
      variable->settingsVar->RevertChange();
    }
  }
  ms->changeMade = false;
  ms->NotifyListeners();
}

void ModSettings::NotifyListeners() {
  std::shared_lock<RED4ext::SharedMutex> _(listeners_lock);
  for (const auto &listener : listeners) {
    if (!listener.Expired() && listener.instance) {
      auto h = (RED4ext::IScriptable*)listener.instance;
      //RED4ext::StackArgs_t args = RED4ext::StackArgs_t();
      auto onChange = h->GetType()->GetFunction("OnModSettingsChange");
      if (onChange) {
        auto stack = RED4ext::CStack(h);
        onChange->Execute(&stack);
      }
      //h.refCount->DecRef();
    }
  }
}

void RegisterListenerToModificationsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                    int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> instance;
  RED4ext::GetParameter(aFrame, &instance);
  aFrame->code++;

  if (instance) {
    auto ms = ModSettings::GetInstance();
    std::shared_lock<RED4ext::SharedMutex> _(ms->listeners_lock);
    ms->listeners.EmplaceBack(RED4ext::WeakHandle(instance));
  }
}

void UnregisterListenerToModificationsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                      int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> handle;
  RED4ext::GetParameter(aFrame, &handle);
  aFrame->code++;

  if (handle) {
    auto ms = ModSettings::GetInstance();
    std::shared_lock<RED4ext::SharedMutex> _(ms->listeners_lock);
    auto i = 0;
    for (const auto &listener : ms->listeners) {
      if (listener.instance == handle.instance) {
        ms->listeners.RemoveAt(i);
        break;
      }
      i++;
    }
  }
}

void RegisterListenerToClassScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                    int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> instance;
  RED4ext::GetParameter(aFrame, &instance);
  aFrame->code++;

  if (instance) {
    auto ms = ModSettings::GetInstance();
    auto className = instance->GetType()->GetName();
    std::shared_lock<RED4ext::SharedMutex> _(ms->variables_lock);
    auto vars_p = ms->variablesByClass.Get(className);
    if (vars_p) {
      auto vars = *vars_p;
      for (auto i = 0; i < vars.size; i++) {
        std::shared_lock<RED4ext::SharedMutex> _(vars[i]->listeners_lock);
        vars[i]->listeners.EmplaceBack(RED4ext::WeakHandle(instance));
      }
    }
  }
}

void UnregisterListenerToClassScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                    int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> handle;
  RED4ext::GetParameter(aFrame, &handle);
  aFrame->code++;

  if (handle) {
    auto ms = ModSettings::GetInstance();
    auto className = handle->GetType()->GetName();
    auto vars = ms->variablesByClass.Get(className);
    if (vars) {
      for (auto &var : *vars) {
        auto i = 0;
        for (const auto &listener : var->listeners) {
          std::shared_lock<RED4ext::SharedMutex> _(var->listeners_lock);
          if (listener.instance == handle.instance) {
            var->listeners.RemoveAt(i);
            break;
          }
          i++;
        }
      }
    }
  }
}

void ModSettings::RegisterTypes() {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto scriptable = rtti->GetClass("IScriptable");
  modSettings.flags = {.isNative = true};
  modSettings.parent = scriptable;
  RED4ext::CRTTISystem::Get()->RegisterType(&modSettings);
};

void ModSettings::PostRegisterTypes() {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto getInstance = RED4ext::CClassStaticFunction::Create(&modSettings, "GetInstance", "GetInstance",
                                                           &GetInstanceScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(getInstance);
  auto getMods = RED4ext::CClassStaticFunction::Create(&modSettings, "GetMods", "GetMods", &GetModsScripts,
                                                       {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(getMods);
  auto getVars = RED4ext::CClassStaticFunction::Create(&modSettings, "GetVars", "GetVars", &GetVarsScripts,
                                                       {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(getVars);
  auto getCategories = RED4ext::CClassStaticFunction::Create(
      &modSettings, "GetCategories", "GetCategories", &GetCategoriesScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(getCategories);
  auto acceptChanges = RED4ext::CClassStaticFunction::Create(
      &modSettings, "AcceptChanges", "AcceptChanges", &AcceptChangesScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(acceptChanges);
  auto rejectChanges = RED4ext::CClassStaticFunction::Create(
      &modSettings, "RejectChanges", "RejectChanges", &RejectChangesScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(rejectChanges);
  auto restoreDefaults = RED4ext::CClassStaticFunction::Create(
      &modSettings, "RestoreDefaults", "RestoreDefaults", &RestoreDefaultsScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(restoreDefaults);
  auto registerListenerToClass =
      RED4ext::CClassStaticFunction::Create(&modSettings, "RegisterListenerToClass", "RegisterListenerToClass",
                                            &RegisterListenerToClassScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(registerListenerToClass);
  auto unregisterListenerToClass =
      RED4ext::CClassStaticFunction::Create(&modSettings, "UnregisterListenerToClass", "UnregisterListenerToClass",
                                            &UnregisterListenerToClassScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(unregisterListenerToClass);
  auto registerListenerToModifications =
      RED4ext::CClassStaticFunction::Create(&modSettings, "RegisterListenerToModifications", "RegisterListenerToModifications",
                                            &RegisterListenerToModificationsScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(registerListenerToModifications);
  auto unregisterListenerToModifications =
      RED4ext::CClassStaticFunction::Create(&modSettings, "UnregisterListenerToModifications", "UnregisterListenerToModifications",
                                            &UnregisterListenerToModificationsScripts, {.isNative = true, .isStatic = true});
  modSettings.RegisterFunction(unregisterListenerToModifications);

  modSettings.props.PushBack(
      RED4ext::CProperty::Create(rtti->GetType("Bool"), "changeMade", nullptr, offsetof(ModSettings, changeMade)));
};