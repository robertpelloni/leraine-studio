#pragma once

#include "base/module.h"
#include <map>
#include <vector>

struct ShortcutDefinition
{
    std::string ActionName;
    sf::Keyboard::Key ModifierKey = sf::Keyboard::Unknown;
    sf::Keyboard::Key ActionKey = sf::Keyboard::Unknown;
};

class ShortcutMenuModule : public Module
{
public:

    virtual bool ProcessEvent(const sf::Event& InEvent) override;

public:

    bool BeginMenu(const std::string InLabel);
    bool MenuItem(const std::string InLabel, sf::Keyboard::Key InModifierKey, sf::Keyboard::Key InActionKey);
    void EndMenu();
    void Separator();

    void ShowCheatSheet();

    void RegisterShortcut(const std::string& InAction, sf::Keyboard::Key InModifier, sf::Keyboard::Key InActionKey);
    bool UpdateShortcut(const std::string& InAction, sf::Keyboard::Key InModifier, sf::Keyboard::Key InActionKey);
    std::string GetShortcutString(const std::string& InAction) const;
    const std::map<std::string, ShortcutDefinition>& GetAllShortcuts() const { return _Shortcuts; }

private:

    bool _ImGuiMenuBegin = false;
    bool _ShouldCaptureShortcut = false;

    bool _CtrlKey = false;
	bool _ShiftKey = false;
	bool _AltKey = false;

    bool _KeyboardPressedStates[sf::Keyboard::KeyCount];

    std::map<std::string, ShortcutDefinition> _Shortcuts;
};