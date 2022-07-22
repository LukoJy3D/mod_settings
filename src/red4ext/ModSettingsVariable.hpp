#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Common.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/RuntimeSettingsVar.hpp>

struct ModRuntimeSettingsVar;

struct ModSettingsVariable {
  RED4ext::CName mod = RED4ext::CName();
  RED4ext::CName typeName = RED4ext::CName();
  RED4ext::CName className = RED4ext::CName();
  RED4ext::CName category = RED4ext::CName();
  ModRuntimeSettingsVar *settingsVar;
  RED4ext::DynArray<RED4ext::IScriptable*> listeners;
  RED4ext::SharedMutex listeners_lock;

  void UpdateValues();
};