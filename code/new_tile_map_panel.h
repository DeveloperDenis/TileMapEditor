#ifndef NEW_TILE_MAP_PANEL_H_
#define NEW_TILE_MAP_PANEL_H_

struct NewTileMapPanelData
{
    char *tileMapName;
    int tileSize;
    int widthInTiles;
    int heightInTiles;
};

void createNewTileMapPanel(int startX, int startY, int maxWidth, int maxHeight);

void newTileMapPanelSetPosition(Vector2 newPos);
int newTileMapPanelGetWidth();
int newTileMapPanelGetHeight();

void newTileMapPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton);
void newTileMapPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton);

void newTileMapPanelSelectNext();
void newTileMapPanelSelectPrevious();
void newTileMapPanelEnterPressed();

void newTileMapPanelCharInput(char c);
void newTileMapPanelCharDeleted();

bool newTileMapPanelVisible();
void newTileMapPanelSetVisible(bool newValue);

bool newTileMapPanelDataReady();
NewTileMapPanelData* newTileMapPanelGetData();

void newTileMapPanelDraw();

void newTileMapPanelSetTileSize(int32 newSize);
int32 newTileMapPanelGetTileSize();

#endif
