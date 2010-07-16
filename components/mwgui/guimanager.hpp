#ifndef GAME_SOUND_GUIMANAGER_H
#define GAME_SOUND_GUIMANAGER_H

namespace MWGui
{
  // Predeclarations, these are defined in mw_layouts.hpp
    class HUD;
    class MapWindow;
    class MainMenu;
    class StatsWindow;

    class GuiManager
    {
        HUD *hud;
        MapWindow *map;
        MainMenu *menu;
        StatsWindow *stats;

        public:
        
            enum GuiWindow
            {
                Gui_Inventory, Gui_Magic, Gui_Map, Gui_Status
            };
            
            enum GuiOneTimeDialogue // used only once
            {
                // character generation
                Gui_Birth, Gui_Class, Gui_Name, Gui_Race, Gui_Review
            };
                
            enum GuiDialogue
            {                
                Gui_Rest
            };

            void enableWindow (GuiWindow window);
            ///< diabled by default.
            
            void showOneTimeDialogue (GuiOneTimeDialogue dialogue);
            
            void enableDialogue (GuiDialogue dialogue);
            ///< disabled by default.
            
            void showDialogue (GuiDialogue dialogue);
            
            bool isGuiActive() const;
            ///< Any non-HUD GUI element active (dialogues and windows)?
    };
}

#endif