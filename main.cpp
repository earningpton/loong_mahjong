#include <iostream>
#include <raylib.h>
#include <deque>
#include <raymath.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <utility>

#ifdef PLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace std;


// Define missing color constants
#ifndef LIGHTBLUE
#define LIGHTBLUE (Color){173, 216, 230, 255}
#endif
#ifndef LIGHTGREEN
#define LIGHTGREEN (Color){144, 238, 144, 255}
#endif

// Game states
enum GameState {
    TITLE_SCREEN,
    LOONG_SELECTION,
    DIFFICULTY_SELECTION,
    INSTRUCTION_SCREEN,
    COUNTDOWN,
    PLAYING,
    GAME_OVER,
    CULTIVATION_SUCCESS,
    LOONG_UPGRADE
};

// Cultivation Difficulty Levels
enum DifficultyLevel {
    FOUNDATION_BUILDING = 0,  // 3 Extra Lives
    GOLDEN_CORE = 1,          // 1 Life, no Extra Lives
    NASCENT_SOUL = 2,         // 7->10->13 tiles progression
    ASCENSION = 3,            // 10 tiles, 5 Mahjongs to win
    IMMORTAL_SAGE = 4         // 13 tiles, 5 Mahjongs to win
};

// LOONG Types - Each with unique abilities and upgrade paths
enum LoongType {
    BASIC_LOONG = 0,        // Balanced starter dragon
    FIRE_LOONG = 1,         // Aggressive damage dealer
    WATER_LOONG = 2,        // Defensive and healing
    WHITE_LOONG = 3,        // Pure and purifying
    EARTH_LOONG = 4,        // Slow but powerful
    WIND_LOONG = 5,         // Fast and agile
    SHADOW_LOONG = 6,       // Mysterious and tricky
    CELESTIAL_LOONG = 7,    // Ultimate endgame dragon
    PATIENCE_LOONG = 8      // Dragon of patience and control
};

struct LoongUpgrade {
    string name;
    string description;
    int level;
    int maxLevel;

    LoongUpgrade(string n, string desc, int maxLvl) : name(n), description(desc), level(0), maxLevel(maxLvl) {}
};

struct LoongData {
    LoongType type;
    string name;
    string description;
    Color primaryColor;
    Color secondaryColor;
    vector<LoongUpgrade> upgrades;
    int level; // Overall LOONG level (increases every 10 snake age)

    LoongData(LoongType t, string n, string desc, Color primary, Color secondary)
        : type(t), name(n), description(desc), primaryColor(primary), secondaryColor(secondary), level(0) {}
};

// Ornate levels
enum OrnateLevel {
    LEVEL_1_NONE = 0,
    LEVEL_2_BASIC = 1,
    LEVEL_3_INTRICATE = 2,
    LEVEL_4_DRAGON = 3,
    LEVEL_5_GOLD_BASIC = 4,
    LEVEL_6_GOLD_INTRICATE = 5,
    LEVEL_7_GOLD_DRAGON = 6,
    LEVEL_8_ORNATE_GOLD = 7,
    LEVEL_9_MASTER_GOLD = 8,
    LEVEL_10_LEGENDARY = 9
};

// Game area settings (inner box where snake moves)
int cellSize = 40;
int cellCount = 20;
int gameAreaOffset = 150;

// Total canvas settings (outer area for mouse control + UI)
int canvasWidth = 1800;  // Increased further to prevent UI overlap
int canvasHeight = 1000;

// UI panel settings
int uiPanelX = 1400;  // Shifted right to give game area more room
int uiPanelWidth = 400;

double lastUpdateTime = 0;

// Function to draw simplified ornate background pattern based on level
void DrawOrnateBackground(int ornateLevel) {
    // Level 0: Plain background (no patterns)
    if (ornateLevel == 0) return;

    // Level 1: Simple white lines
    Color level1Color = {255, 255, 255, 80}; // Light white

    // Level 2: More white lines
    Color level2Color = {255, 255, 255, 120}; // Medium white

    // Level 3: Gold and white crossing pattern
    Color goldColor = {255, 215, 0, 150}; // Gold
    Color whiteColor = {255, 255, 255, 100}; // White

    if (ornateLevel >= 1) {
        // Level 1: Simple white lines
        for (int x = 0; x < canvasWidth; x += 100) {
            DrawLineEx({(float)x, 0}, {(float)x, (float)canvasHeight}, 2, level1Color);
        }
        for (int y = 0; y < canvasHeight; y += 100) {
            DrawLineEx({0, (float)y}, {(float)canvasWidth, (float)y}, 2, level1Color);
        }
    }

    if (ornateLevel >= 2) {
        // Level 2: More white lines (diagonal)
        for (int x = 0; x < canvasWidth; x += 80) {
            for (int y = 0; y < canvasHeight; y += 80) {
                DrawLineEx({(float)x, (float)y}, {(float)x + 80, (float)y + 80}, 2, level2Color);
                DrawLineEx({(float)x + 80, (float)y}, {(float)x, (float)y + 80}, 2, level2Color);
            }
        }
    }

    if (ornateLevel >= 3) {
        // Level 3: Beautiful gold and white crossing pattern
        for (int x = 0; x < canvasWidth; x += 60) {
            for (int y = 0; y < canvasHeight; y += 60) {
                // Gold diamond pattern
                DrawLineEx({(float)x + 30, (float)y}, {(float)x + 60, (float)y + 30}, 3, goldColor);
                DrawLineEx({(float)x + 60, (float)y + 30}, {(float)x + 30, (float)y + 60}, 3, goldColor);
                DrawLineEx({(float)x + 30, (float)y + 60}, {(float)x, (float)y + 30}, 3, goldColor);
                DrawLineEx({(float)x, (float)y + 30}, {(float)x + 30, (float)y}, 3, goldColor);

                // White crossing lines
                DrawLineEx({(float)x + 15, (float)y + 15}, {(float)x + 45, (float)y + 45}, 2, whiteColor);
                DrawLineEx({(float)x + 45, (float)y + 15}, {(float)x + 15, (float)y + 45}, 2, whiteColor);
            }
        }
    }
}

// Function to create triumphant "Mahjong" sound effect
void PlayTriumphantMahjongSound(Sound baseSound) {
    // Play triumphant higher pitch sound
    SetSoundPitch(baseSound, 1.8f);
    PlaySound(baseSound);
}

// Performance-optimized sound playing function
void PlaySoundSafe(Sound sound, float pitch, float currentTime, float& lastSoundTime, float cooldown = 0.1f) {
    if (currentTime - lastSoundTime >= cooldown) {
        static float lastPitch = 1.0f;
        if (abs(lastPitch - pitch) > 0.01f) { // Only change pitch if significantly different
            SetSoundPitch(sound, pitch);
            lastPitch = pitch;
        }
        PlaySound(sound);
        lastSoundTime = currentTime;
    }
}



// Mahjong tile system
// Tile types for expanded mahjong
enum TileType {
    PLAIN_TILES = 0,     // 1-9 white border (default)
    HAT_TILES = 1,       // 1-9 with hat symbol
    DOT_TILES = 2        // 1-9 with dot symbol
};

struct Tile {
    int value;      // 0-9 (0 = blank/zero tile)
    TileType type;  // NUMBERS, LETTERS, or DOTS
    bool isGold;    // True if this tile is from a KONG

    Tile(int v = 1, TileType t = PLAIN_TILES, bool gold = false) : value(v), type(t), isGold(gold) {}

    bool operator==(const Tile& other) const {
        return value == other.value && type == other.type;
    }

    string ToString() const {
        if (value == 0) {
            return "0"; // Zero/blank tile
        }
        string result = to_string(value);
        if (type == HAT_TILES) {
            result += "^"; // Hat symbol
        } else if (type == DOT_TILES) {
            result += "."; // Dot symbol
        }
        return result;
    }

    bool IsZero() const {
        return value == 0;
    }
};

// Enhanced tile popup system - NOW AFTER Tile definitions
struct NumberPopup {
    Tile tile;
    Vector2 position;
    float timer;
    bool active;

    NumberPopup() : tile(1, PLAIN_TILES, false), position({0, 0}), timer(0), active(false) {}

    void Show(const Tile& newTile, Vector2 pos) {
        tile = newTile;
        position = pos;
        timer = 1.2f; // Show for 1.2 seconds for better visibility
        active = true;
    }

    void Update(float deltaTime) {
        if (active) {
            timer -= deltaTime;
            if (timer <= 0) {
                active = false;
            }
        }
    }

    void Draw() {
        if (active) {
            // Draw enhanced tile popup with thick colored border
            Rectangle tileRect = {position.x - 40, position.y - 30, 80, 60};
            DrawRectangleRec(tileRect, BLACK);

            // Thick border color based on tile type
            Color borderColor = WHITE;
            if (tile.type == HAT_TILES) borderColor = LIGHTBLUE;
            else if (tile.type == DOT_TILES) borderColor = LIGHTGREEN;

            DrawRectangleLinesEx(tileRect, 6, borderColor); // Extra thick border

            // Draw "PICKED UP:" label above
            DrawText("PICKED UP:", position.x - 45, position.y - 55, 16, YELLOW);

            // Draw tile content with type indicator
            string tileText = to_string(tile.value);
            if (tile.type == HAT_TILES) tileText += "^";
            else if (tile.type == DOT_TILES) tileText += ".";

            // Large text with type-specific color
            Color textColor = WHITE;
            if (tile.type == HAT_TILES) textColor = LIGHTBLUE;
            else if (tile.type == DOT_TILES) textColor = LIGHTGREEN;

            int textWidth = MeasureText(tileText.c_str(), 36);
            DrawText(tileText.c_str(), position.x - textWidth/2, position.y - 18, 36, textColor);

            // Add pulsing effect for extra visibility
            float pulseAlpha = (sin(timer * 8) + 1) * 0.3f + 0.4f; // Pulse between 0.4 and 1.0
            Color pulseColor = borderColor;
            pulseColor.a = (unsigned char)(255 * pulseAlpha);
            DrawRectangleLinesEx(tileRect, 8, pulseColor); // Outer pulsing border
        }
    }
};

class MahjongTiles {
public:
    vector<Tile> tiles;
    int arrowPosition;
    map<pair<int, TileType>, int> tileCounts; // Track tile usage (value, type) -> count
    set<pair<int, TileType>> goldTiles; // Tiles that are KONG (removed from pool)
    int maxTiles; // Maximum number of tiles (4, 7, 10, 13)
    vector<TileType> availableTypes; // Which tile types are available

    // Tile tracker system
    vector<Tile> discardPile; // All tiles we've discarded (for display)
    map<pair<int, TileType>, int> totalDiscardCounts; // Total count of each tile discarded
    bool canReshuffle; // Whether reshuffle is available
    Tile nextTile; // Preview of next tile to be generated

    // TILE LOCKING SYSTEM (White LOONG Future Sight)
    vector<Tile> lockedFutureTiles; // The 3 locked future tiles
    bool futureTilesLocked; // Are future tiles currently locked?
    int lockedTilesRemaining; // How many locked tiles are left

    MahjongTiles() {
        arrowPosition = 0;
        maxTiles = 4; // Start with 4 tiles
        availableTypes.push_back(PLAIN_TILES); // Start with only plain tiles
        canReshuffle = false; // Start with no reshuffle available

        // Initialize tile locking system
        futureTilesLocked = false;
        lockedTilesRemaining = 0;

        // Initialize tile counts for all types (including zero)
        for (int i = 0; i <= 9; i++) { // Include 0 for zero tiles
            tileCounts[make_pair(i, PLAIN_TILES)] = 0;
            tileCounts[make_pair(i, HAT_TILES)] = 0;
            tileCounts[make_pair(i, DOT_TILES)] = 0;
            totalDiscardCounts[make_pair(i, PLAIN_TILES)] = 0;
            totalDiscardCounts[make_pair(i, HAT_TILES)] = 0;
            totalDiscardCounts[make_pair(i, DOT_TILES)] = 0;
        }
        GenerateRandomTiles(0); // Start with no latent level selected
        GenerateNextTile(); // Generate first preview tile

        // Test the advanced Mahjong algorithm on startup
        // TestMahjongLogic(); // Disabled for release
    }

    void GenerateRandomTiles(int selectedLatentLevel = 0) {
        // Reset tile counts and gold tiles when drawing new hand (but keep used tiles tracker)
        cout << "Resetting tile pool for new hand..." << endl;
        tileCounts.clear();
        goldTiles.clear();

        // Reinitialize tile counts
        for (int i = 1; i <= 9; i++) {
            tileCounts[make_pair(i, PLAIN_TILES)] = 0;
            tileCounts[make_pair(i, HAT_TILES)] = 0;
            tileCounts[make_pair(i, DOT_TILES)] = 0;
        }

        tiles.clear();

        // Add latent cultivation LOONG evolution tiles if a level is selected
        cout << "GenerateRandomTiles called with selectedLatentLevel: " << selectedLatentLevel << endl;
        if (selectedLatentLevel > 0) {
            // This will be implemented by calling a function from the Game class
            // since we need access to the current LOONG type and upgrade tile generation
            cout << "Latent Cultivation Level " << selectedLatentLevel << " selected - evolution tiles will be added by Game class" << endl;
        }

        // Fill remaining slots with normal tiles
        for (int i = tiles.size(); i < maxTiles; i++) {
            Tile newTile = GenerateValidTile();
            tiles.push_back(newTile);

            // CRITICAL FIX: Update tile counts for initial hand
            pair<int, TileType> tileKey = {newTile.value, newTile.type};
            tileCounts[tileKey]++;
        }
        SortTiles();
        arrowPosition = 0;
        cout << "Generated " << maxTiles << " tiles: ";
        for (const Tile& tile : tiles) {
            cout << tile.ToString() << " ";
        }
        cout << endl;
    }

    void ShuffleExistingHand() {
        // EARTHQUAKE SHUFFLE: Randomly swap tiles in existing hand without resetting
        cout << "🌍 EARTHQUAKE SHUFFLE: Randomly rearranging existing hand..." << endl;

        if (tiles.size() < 2) return; // Need at least 2 tiles to shuffle

        // Perform multiple random swaps to shuffle the hand
        int numSwaps = max(2, (int)tiles.size() / 2);
        for (int i = 0; i < numSwaps; i++) {
            int index1 = GetRandomValue(0, tiles.size() - 1);
            int index2 = GetRandomValue(0, tiles.size() - 1);

            if (index1 != index2) {
                swap(tiles[index1], tiles[index2]);
                cout << "Swapped positions " << index1 << " and " << index2 << endl;
            }
        }

        // CRITICAL FIX: Always sort after shuffling
        SortTiles();

        cout << "Hand shuffled and sorted! New order: ";
        for (const Tile& tile : tiles) {
            cout << tile.ToString() << " ";
        }
        cout << endl;
    }

    Tile GenerateValidTile(float probabilityBonus = 0.0f) {
        vector<pair<pair<int, TileType>, float>> weightedOptions; // Tile and weight

        // Find all valid tile options with probability weights
        for (TileType type : availableTypes) {
            for (int value = 1; value <= 9; value++) {
                pair<int, TileType> tileKey = {value, type};

                // Skip if this tile is gold (KONG)
                if (goldTiles.count(tileKey) > 0) continue;

                // Skip if we already have 3 of this tile
                if (tileCounts[tileKey] >= 3) continue;

                float weight = 1.0f; // Base weight (always at least 1.0)

                // PROBABILITY BONUS TYPE 1: Held tile boost (for KONG)
                int heldCount = 0;
                for (const Tile& tile : tiles) {
                    if (tile.value == value && tile.type == type) {
                        heldCount++;
                    }
                }
                if (heldCount > 0) {
                    // Cap the bonus to prevent overwhelming base weight
                    float heldBonus = min(3.0f, (probabilityBonus / 100.0f) * heldCount);
                    weight += heldBonus;
                }

                // PROBABILITY BONUS TYPE 2: Adjacent tile boost (for consecutive)
                bool hasAdjacent = false;
                for (const Tile& tile : tiles) {
                    if (tile.type == type) {
                        // Check for adjacent values (±1)
                        if (abs(tile.value - value) == 1) {
                            hasAdjacent = true;
                            break;
                        }
                    }
                }
                if (hasAdjacent) {
                    // Cap the adjacent bonus to prevent overwhelming
                    float adjacentBonus = min(2.0f, probabilityBonus / 200.0f);
                    weight += adjacentBonus;
                }

                weightedOptions.push_back({tileKey, weight});
            }
        }

        if (weightedOptions.empty()) {
            cout << "Warning: No valid tiles available! Auto-reshuffling tile pool..." << endl;

            // Reset tile counts to allow all tiles again
            tileCounts.clear();
            for (TileType type : availableTypes) {
                for (int i = 1; i <= 9; i++) {
                    tileCounts[make_pair(i, type)] = 0;
                }
            }

            // Clear gold tiles (except zeros which can KONG multiple times)
            auto it = goldTiles.begin();
            while (it != goldTiles.end()) {
                if (it->first != 0) { // Keep zero tiles as gold if they were gold
                    it = goldTiles.erase(it);
                } else {
                    ++it;
                }
            }

            cout << "Tile pool reshuffled! Generating new tile..." << endl;

            // Try again with fresh pool
            return GenerateValidTile(probabilityBonus);
        }

        // Weighted random selection
        float totalWeight = 0.0f;
        for (const auto& option : weightedOptions) {
            totalWeight += option.second;
        }

        float randomValue = GetRandomValue(0, (int)(totalWeight * 100)) / 100.0f;
        float currentWeight = 0.0f;

        pair<int, TileType> chosen = weightedOptions[0].first; // Fallback
        for (const auto& option : weightedOptions) {
            currentWeight += option.second;
            if (randomValue <= currentWeight) {
                chosen = option.first;
                break;
            }
        }

        // Increment count
        tileCounts[chosen]++;

        return Tile(chosen.first, chosen.second, false);
    }

    void GenerateNextTile(float probabilityBonus = 0.0f) {
        // Check if we have locked future tiles (White LOONG Future Sight)
        if (futureTilesLocked && !lockedFutureTiles.empty() && lockedTilesRemaining > 0) {
            // Use the next locked tile
            nextTile = lockedFutureTiles[3 - lockedTilesRemaining]; // Get the correct locked tile
            lockedTilesRemaining--;
            cout << "Next tile (LOCKED): " << nextTile.ToString() << " (" << lockedTilesRemaining << " locked tiles remaining)" << endl;

            // If no more locked tiles, disable the system
            if (lockedTilesRemaining <= 0) {
                futureTilesLocked = false;
                lockedFutureTiles.clear();
                cout << ">>> FUTURE SIGHT EXPIRED: No more locked tiles!" << endl;
            }
        } else {
            // Normal tile generation
            nextTile = GenerateValidTile(probabilityBonus);
            cout << "Next tile preview: " << nextTile.ToString() << endl;
        }
    }

    void SortTiles() {
        sort(tiles.begin(), tiles.end(), [](const Tile& a, const Tile& b) {
            if (a.type != b.type) return a.type < b.type; // Sort by type first
            return a.value < b.value; // Then by value
        });
    }

    void MoveArrowLeft() {
        if (arrowPosition > 0) {
            arrowPosition--;
        }
    }

    void MoveArrowRight() {
        if (arrowPosition < maxTiles) {  // Now includes KEEP option (position maxTiles)
            arrowPosition++;
        }
    }

    void MoveArrowToFarLeft() {
        arrowPosition = 0;  // Jump to leftmost position
    }

    void MoveArrowToFarRight() {
        arrowPosition = maxTiles;  // Jump to KEEP position
    }

    void ReplaceTileAtArrow(const Tile& newTile) {
        if (arrowPosition < maxTiles) {  // Replace tile in hand
            // Track the old tile before replacing (goes to discard)
            Tile oldTile = tiles[arrowPosition];
            AddToDiscardPile(oldTile);

            tiles[arrowPosition] = newTile;
            SortTiles();
            // Reset arrow to point to the new tile's position after sorting
            for (int i = 0; i < (int)tiles.size(); i++) {
                if (tiles[i] == newTile) {
                    arrowPosition = i;
                    break;
                }
            }
        } else {
            // KEEP was selected - new tile goes to discard pile
            AddToDiscardPile(newTile);
            cout << "KEEP selected - " << newTile.ToString() << " goes to discard pile" << endl;
        }
    }

    bool IsKeepSelected() {
        return arrowPosition == maxTiles; // KEEP is at position maxTiles
    }

    void SetTileCount(int newCount, int currentOrnateLevel = 0) {
        cout << "SetTileCount called with newCount: " << newCount << endl;
        cout << "Current maxTiles: " << maxTiles << ", current tiles.size(): " << tiles.size() << endl;

        if (newCount >= 4 && newCount <= 13) {
            int oldMaxTiles = maxTiles;
            maxTiles = newCount;

            // Add new tile types based on ornate level (NOT tile count)
            if (newCount == 7 && availableTypes.size() == 1 && currentOrnateLevel >= 1) {
                availableTypes.push_back(HAT_TILES);
                cout << "Added HAT tiles (1^-9^) to available tile types!" << endl;
            } else if (newCount == 10 && availableTypes.size() == 2 && currentOrnateLevel >= 2) {
                availableTypes.push_back(DOT_TILES);
                cout << "Added DOT tiles (1.-9.) to available tile types!" << endl;
            }

            cout << "Tile count expanded to " << maxTiles << endl;
            cout << "Available tile types: " << availableTypes.size() << endl;

            // CRITICAL FIX: Expand existing hand instead of regenerating!
            if (newCount > oldMaxTiles) {
                int tilesToAdd = newCount - tiles.size();
                cout << "EXPANDING HAND: Adding " << tilesToAdd << " new tiles to existing hand" << endl;

                // Add new tiles to existing hand without resetting
                for (int i = 0; i < tilesToAdd; i++) {
                    Tile newTile = GenerateValidTile(0.0f); // No probability bonus for expansion
                    tiles.push_back(newTile);

                    // Update tile counts
                    pair<int, TileType> tileKey = {newTile.value, newTile.type};
                    tileCounts[tileKey]++;

                    cout << "Added tile " << newTile.ToString() << " to hand" << endl;
                }

                // CRITICAL FIX: Always sort after adding new tiles
                SortTiles();
                cout << "Hand sorted after expansion!" << endl;
            }

            cout << "After expansion - tiles.size(): " << tiles.size() << endl;
        } else {
            cout << "Invalid tile count: " << newCount << " (must be 4-13)" << endl;
        }
    }

    void RedrawCompleteHand(int newCount, int currentOrnateLevel = 0) {
        cout << "🎴 COMPLETE HAND REDRAW! " << tiles.size() << " -> " << newCount << " tiles" << endl;

        if (newCount >= 4 && newCount <= 13) {
            maxTiles = newCount;

            // Add new tile types based on ornate level (NOT tile count)
            if (newCount == 7 && availableTypes.size() == 1 && currentOrnateLevel >= 1) {
                availableTypes.push_back(HAT_TILES);
                cout << "Added HAT tiles (1^-9^) to available tile types!" << endl;
            } else if (newCount == 10 && availableTypes.size() == 2 && currentOrnateLevel >= 2) {
                availableTypes.push_back(DOT_TILES);
                cout << "Added DOT tiles (1.-9.) to available tile types!" << endl;
            }

            // COMPLETE REDRAW: Clear existing hand and generate new one
            cout << "🎴 Clearing old hand and generating " << newCount << " fresh tiles..." << endl;

            // Clear old hand and tile counts
            tiles.clear();
            tileCounts.clear();

            // Reinitialize tile counts for all available types
            for (TileType type : availableTypes) {
                for (int i = 1; i <= 9; i++) {
                    tileCounts[make_pair(i, type)] = 0;
                }
            }

            // Generate completely new hand
            for (int i = 0; i < newCount; i++) {
                Tile newTile = GenerateValidTile(0.0f);
                tiles.push_back(newTile);

                // CRITICAL FIX: Update tile counts for the new hand
                pair<int, TileType> tileKey = {newTile.value, newTile.type};
                tileCounts[tileKey]++;

                cout << "Generated fresh tile: " << newTile.ToString() << endl;
            }

            // Sort the new hand
            SortTiles();
            cout << "🎴 HAND COMPLETELY REDRAWN AND SORTED! New hand: ";
            for (const Tile& tile : tiles) {
                cout << tile.ToString() << " ";
            }
            cout << endl;
        } else {
            cout << "Invalid tile count: " << newCount << " (must be 4-13)" << endl;
        }
    }

    bool CheckWinCondition(const Tile& newTile, int currentOrnateLevel = 0) {
        vector<Tile> testTiles = tiles;
        testTiles.push_back(newTile);
        SortTilesVector(testTiles);

        // Win condition check (debug output removed for release)

        // Calculate required groups based on hand size
        int handSize = testTiles.size();
        int requiredGroups = 0;
        if (handSize == 5) requiredGroups = 1; // 4 tiles: 1 group + 1 pair
        else if (handSize == 8) requiredGroups = 2; // 7 tiles: 2 groups + 1 pair
        else if (handSize == 11) requiredGroups = 3; // 10 tiles: 3 groups + 1 pair
        else if (handSize == 14) requiredGroups = 4; // 13 tiles: 4 groups + 1 pair
        else {
            cout << "Invalid hand size: " << handSize << endl;
            return false;
        }

        bool mahjongWin = HasRequiredGroupsAndPair(testTiles, requiredGroups);
        bool loongWin = false;

        // Check for LOONG (1-9 consecutive of same type) - only after level 4
        if (currentOrnateLevel >= LEVEL_4_DRAGON) { // Level 4+ (ornateLevel is 0-based)
            loongWin = HasLoongWin(testTiles);
        }

        if (mahjongWin) {
            cout << "WIN: Found " << requiredGroups << " groups + 1 pair!" << endl;
        }
        if (loongWin) {
            cout << "LOONG WIN: Found 1-9 consecutive of same type!" << endl;
        }

        return mahjongWin || loongWin;
    }

    void SortTilesVector(vector<Tile>& tilesToSort) {
        sort(tilesToSort.begin(), tilesToSort.end(), [](const Tile& a, const Tile& b) {
            if (a.type != b.type) return a.type < b.type;
            return a.value < b.value;
        });
    }

    bool CheckKongCondition(const Tile& newTile) {
        // First check if we have 3 of this tile (same value and type) in current hand
        int countInHand = 0;
        for (const Tile& tile : tiles) {
            if (tile == newTile) {
                countInHand++;
            }
        }

        // KONG only triggers if we have exactly 3 of this tile in hand + 1 new = 4 total
        if (countInHand == 3) {
            cout << "KONG: Found 3x" << newTile.ToString() << " in hand + 1 new = KONG!" << endl;

            // SPECIAL CASE: Zero tiles can KONG multiple times (don't make them GOLD)
            if (!newTile.IsZero()) {
                // Make this tile type GOLD (remove from pool) - except for zeros
                pair<int, TileType> tileKey = {newTile.value, newTile.type};
                goldTiles.insert(tileKey);
                cout << "Tile " << newTile.ToString() << " is now GOLD and removed from pool!" << endl;
            } else {
                cout << "Zero tile KONG! Zeros can KONG multiple times!" << endl;
            }

            // Mark all instances in hand as gold
            for (Tile& tile : tiles) {
                if (tile == newTile) {
                    tile.isGold = true;
                }
            }

            return true;
        }
        return false;
    }

    bool HasRequiredGroupsAndPair(vector<Tile>& sortedTiles, int requiredGroups) {
        // Simple approach: count all possible groups and pairs, then check if we can form the required combination
        map<pair<int, TileType>, int> tileCounts;
        for (const Tile& tile : sortedTiles) {
            tileCounts[{tile.value, tile.type}]++;
        }

        cout << "Checking for " << requiredGroups << " groups + 1 pair in hand of " << sortedTiles.size() << " tiles" << endl;

        // Try all possible combinations of groups
        return TryAllGroupCombinations(tileCounts, requiredGroups, sortedTiles.size());
    }

    bool TryAllGroupCombinations(map<pair<int, TileType>, int> tileCounts, int requiredGroups, int totalTiles) {
        // Mahjong logic processing (debug output removed for release)

        // Use recursive backtracking to try ALL possible combinations
        return TryAllPossibleCombinations(tileCounts, requiredGroups, 0, vector<string>());
    }

    // ADVANCED RECURSIVE MAHJONG ALGORITHM - Handles ALL combinations!
    bool TryAllPossibleCombinations(map<pair<int, TileType>, int> tileCounts, int requiredGroups, int groupsFound, vector<string> foundGroups) {
        // Base case: if we have enough groups, check for exactly one pair
        if (groupsFound >= requiredGroups) {
            int remainingTiles = 0;
            int pairsFound = 0;

            for (auto& entry : tileCounts) {
                if (entry.second > 0) {
                    remainingTiles += entry.second;
                    if (entry.second == 2) {
                        pairsFound++;
                    } else if (entry.second > 2) {
                        // More than 2 tiles remaining means invalid
                        return false;
                    }
                }
            }

            bool validMahjong = (pairsFound == 1 && remainingTiles == 2);

            // Valid Mahjong found (debug output removed for release)

            return validMahjong;
        }

        // Try to form triplets
        for (auto& entry : tileCounts) {
            if (entry.second >= 3) {
                // Make a copy and try this triplet
                map<pair<int, TileType>, int> newCounts = tileCounts;
                newCounts[entry.first] -= 3;

                vector<string> newGroups = foundGroups;
                string tripletName = "Triplet " + to_string(entry.first.first);
                if (entry.first.second == HAT_TILES) tripletName += "^";
                else if (entry.first.second == DOT_TILES) tripletName += ".";
                newGroups.push_back(tripletName);

                if (TryAllPossibleCombinations(newCounts, requiredGroups, groupsFound + 1, newGroups)) {
                    return true;
                }
            }
        }

        // Try to form consecutives
        for (TileType type : {PLAIN_TILES, HAT_TILES, DOT_TILES}) {
            for (int start = 1; start <= 7; start++) { // 1-2-3 to 7-8-9
                pair<int, TileType> tile1 = {start, type};
                pair<int, TileType> tile2 = {start + 1, type};
                pair<int, TileType> tile3 = {start + 2, type};

                if (tileCounts[tile1] > 0 && tileCounts[tile2] > 0 && tileCounts[tile3] > 0) {
                    // Make a copy and try this consecutive
                    map<pair<int, TileType>, int> newCounts = tileCounts;
                    newCounts[tile1]--;
                    newCounts[tile2]--;
                    newCounts[tile3]--;

                    vector<string> newGroups = foundGroups;
                    string consecutiveName = "Consecutive " + to_string(start) + "-" + to_string(start+1) + "-" + to_string(start+2);
                    if (type == HAT_TILES) consecutiveName += "^";
                    else if (type == DOT_TILES) consecutiveName += ".";
                    newGroups.push_back(consecutiveName);

                    if (TryAllPossibleCombinations(newCounts, requiredGroups, groupsFound + 1, newGroups)) {
                        return true;
                    }
                }
            }
        }

        return false; // No valid combination found
    }

    // Test function for Mahjong logic - can be called for debugging
    void TestMahjongLogic() {
        cout << "\n=== TESTING MAHJONG LOGIC ===" << endl;

        // Test case 1: 4-4-5-6-7 (pair + consecutive)
        map<pair<int, TileType>, int> testCase1;
        testCase1[{4, PLAIN_TILES}] = 2; // Pair of 4s
        testCase1[{5, PLAIN_TILES}] = 1;
        testCase1[{6, PLAIN_TILES}] = 1;
        testCase1[{7, PLAIN_TILES}] = 1;

        cout << "Test 1 - 4-4-5-6-7: ";
        bool result1 = TryAllGroupCombinations(testCase1, 1, 5);
        cout << (result1 ? "PASS" : "FAIL") << endl;

        // Test case 2: 1-1-1-2-3 (triplet + consecutive)
        map<pair<int, TileType>, int> testCase2;
        testCase2[{1, PLAIN_TILES}] = 3; // Triplet of 1s
        testCase2[{2, PLAIN_TILES}] = 1;
        testCase2[{3, PLAIN_TILES}] = 1;

        cout << "Test 2 - 1-1-1-2-3: ";
        bool result2 = TryAllGroupCombinations(testCase2, 1, 5);
        cout << (result2 ? "FAIL (should be invalid)" : "PASS") << endl;

        // Test case 3: 1-1-1-2-2 (triplet + pair)
        map<pair<int, TileType>, int> testCase3;
        testCase3[{1, PLAIN_TILES}] = 3; // Triplet of 1s
        testCase3[{2, PLAIN_TILES}] = 2; // Pair of 2s

        cout << "Test 3 - 1-1-1-2-2: ";
        bool result3 = TryAllGroupCombinations(testCase3, 1, 5);
        cout << (result3 ? "PASS" : "FAIL") << endl;

        cout << "=== MAHJONG LOGIC TESTS COMPLETE ===" << endl;
    }



    bool HasLoongWin(vector<Tile>& sortedTiles) {
        // Check for LOONG: 1-9 consecutive tiles of the SAME type
        map<TileType, set<int>> typeValues;
        for (const Tile& tile : sortedTiles) {
            typeValues[tile.type].insert(tile.value);
        }

        // Check each type for 1-9 consecutive
        for (auto& typeGroup : typeValues) {
            set<int>& values = typeGroup.second;
            if (values.size() == 9) {
                // Check if we have exactly 1,2,3,4,5,6,7,8,9
                bool isConsecutive = true;
                for (int i = 1; i <= 9; i++) {
                    if (values.count(i) == 0) {
                        isConsecutive = false;
                        break;
                    }
                }
                if (isConsecutive) {
                    // LOONG WIN found (debug output removed for release)
                    return true;
                }
            }
        }
        return false;
    }

    void AddToDiscardPile(const Tile& tile) {
        // Add to discard pile
        discardPile.push_back(tile);

        // Update total discard counts
        pair<int, TileType> tileKey = {tile.value, tile.type};
        totalDiscardCounts[tileKey]++;

        // Enable reshuffle if we have discarded tiles
        canReshuffle = true;

        // Tile added to discard pile (debug output removed for release)
    }

    void ReshuffleTiles() {
        if (!canReshuffle) {
            // No reshuffle available (debug output removed for release)
            return;
        }

        // Reshuffling tiles (debug output removed for release)

        // Reset tile counts to 0, but respect current hand and KONG tiles
        for (int i = 1; i <= 9; i++) {
            for (TileType type : {PLAIN_TILES, HAT_TILES, DOT_TILES}) {
                pair<int, TileType> tileKey = {i, type};
                tileCounts[tileKey] = 0;

                // Count tiles in current hand
                for (const Tile& tile : tiles) {
                    if (tile.value == i && tile.type == type) {
                        tileCounts[tileKey]++;
                    }
                }

                // KONG tiles count as 4 (max)
                if (goldTiles.count(tileKey) > 0) {
                    tileCounts[tileKey] = 4; // Max out KONG tiles
                }
            }
        }

        // Clear discard pile and reset counts
        discardPile.clear();
        for (auto& entry : totalDiscardCounts) {
            entry.second = 0;
        }

        canReshuffle = false;
        // Tile pool reset (debug output removed for release)
    }

    vector<Tile> GetSortedDiscardPile() {
        vector<Tile> sorted = discardPile;
        sort(sorted.begin(), sorted.end(), [](const Tile& a, const Tile& b) {
            if (a.type != b.type) return a.type < b.type;
            return a.value < b.value;
        });
        return sorted;
    }

    vector<pair<pair<int, TileType>, int>> GetRemainingTiles() {
        vector<pair<pair<int, TileType>, int>> remaining;

        for (TileType type : availableTypes) {
            for (int value = 1; value <= 9; value++) {
                pair<int, TileType> tileKey = {value, type};

                // Calculate remaining count: 4 - current usage
                int currentCount = tileCounts[tileKey];
                int remainingCount = 4 - currentCount;

                // KONG tiles are maxed out (0 remaining)
                if (goldTiles.count(tileKey) > 0) {
                    remainingCount = 0;
                }

                if (remainingCount > 0) {
                    remaining.push_back({tileKey, remainingCount});
                }
            }
        }

        return remaining;
    }

private:
};

bool ElementInDeque(Vector2 element, deque<Vector2> deque)
{
    for (unsigned int i = 0; i < deque.size(); i++)
    {
        if (Vector2Equals(deque[i], element))
        {
            return true;
        }
    }
    return false;
}

bool EventTriggered(double interval)
{
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval)
    {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

class Snake
{
public:
    deque<Vector2> body = {Vector2{10, 10}, Vector2{9, 10}, Vector2{8, 10}, Vector2{7, 10}};
    Vector2 direction = {1, 0};
    bool addSegment = false;
    int segmentsToAdd = 0;

    void Draw(Color bodyColor = {34, 139, 34, 255}, Color scaleColor = {144, 238, 144, 255})
    {
        for (unsigned int i = 0; i < body.size(); i++)
        {
            float x = body[i].x;
            float y = body[i].y;
            float centerX = gameAreaOffset + x * cellSize + cellSize / 2;
            float centerY = gameAreaOffset + y * cellSize + cellSize / 2;

            if (i == 0) {
                // DRAGON HEAD - Triangle with mustache for authentic LOONG look!
                // Use dragon-specific colors for head (brighter than body)
                Color headColor = bodyColor;
                headColor.r = min(255, headColor.r + 50); // Make head brighter than body
                headColor.g = min(255, headColor.g + 50);
                headColor.b = min(255, headColor.b + 50);
                Color accentColor = scaleColor; // Use scale color for accents

                // Main triangular head pointing in movement direction
                Vector2 tip, left, right;
                float headSize = cellSize * 0.4f;

                // FIXED: Ensure proper triangle winding for all directions
                if (direction.x == 1) { // Moving right
                    tip = {centerX + headSize, centerY};
                    left = {centerX - headSize/2, centerY - headSize};
                    right = {centerX - headSize/2, centerY + headSize};
                } else if (direction.x == -1) { // Moving left
                    tip = {centerX - headSize, centerY};
                    left = {centerX + headSize/2, centerY + headSize}; // FIXED: Swapped order
                    right = {centerX + headSize/2, centerY - headSize};
                } else if (direction.y == 1) { // Moving down
                    tip = {centerX, centerY + headSize};
                    left = {centerX + headSize, centerY - headSize/2}; // FIXED: Swapped order
                    right = {centerX - headSize, centerY - headSize/2};
                } else { // Moving up
                    tip = {centerX, centerY - headSize};
                    left = {centerX - headSize, centerY + headSize/2};
                    right = {centerX + headSize, centerY + headSize/2};
                }

                // Draw dragon head triangle with consistent color
                DrawTriangle(tip, left, right, headColor);
                DrawTriangleLines(tip, left, right, BLACK);

                // Dragon eyes (small black circles)
                float eyeOffset = headSize * 0.3f;
                if (direction.x != 0) {
                    DrawCircle(centerX - direction.x * eyeOffset/2, centerY - eyeOffset/2, 3, BLACK);
                    DrawCircle(centerX - direction.x * eyeOffset/2, centerY + eyeOffset/2, 3, BLACK);
                } else {
                    DrawCircle(centerX - eyeOffset/2, centerY - direction.y * eyeOffset/2, 3, BLACK);
                    DrawCircle(centerX + eyeOffset/2, centerY - direction.y * eyeOffset/2, 3, BLACK);
                }

                // Dragon mustache/whiskers (accent color lines)
                float whiskerLength = headSize * 0.8f;
                if (direction.x == 1) { // Right
                    DrawLineEx({centerX - headSize/3, centerY - headSize/2}, {centerX - headSize/3 - whiskerLength, centerY - headSize}, 2, accentColor);
                    DrawLineEx({centerX - headSize/3, centerY + headSize/2}, {centerX - headSize/3 - whiskerLength, centerY + headSize}, 2, accentColor);
                } else if (direction.x == -1) { // Left
                    DrawLineEx({centerX + headSize/3, centerY - headSize/2}, {centerX + headSize/3 + whiskerLength, centerY - headSize}, 2, accentColor);
                    DrawLineEx({centerX + headSize/3, centerY + headSize/2}, {centerX + headSize/3 + whiskerLength, centerY + headSize}, 2, accentColor);
                } else if (direction.y == 1) { // Down
                    DrawLineEx({centerX - headSize/2, centerY - headSize/3}, {centerX - headSize, centerY - headSize/3 - whiskerLength}, 2, accentColor);
                    DrawLineEx({centerX + headSize/2, centerY - headSize/3}, {centerX + headSize, centerY - headSize/3 - whiskerLength}, 2, accentColor);
                } else { // Up
                    DrawLineEx({centerX - headSize/2, centerY + headSize/3}, {centerX - headSize, centerY + headSize/3 + whiskerLength}, 2, accentColor);
                    DrawLineEx({centerX + headSize/2, centerY + headSize/3}, {centerX + headSize, centerY + headSize/3 + whiskerLength}, 2, accentColor);
                }

            } else if (i == 1 || i % 5 == 0) { // Dragon arms behind head and every 5th segment
                // DRAGON BODY WITH ARMS - Slimmer, more serpentine with dragon-specific colors
                float bodyWidth = cellSize * 0.6f; // Slimmer than head
                float bodyHeight = cellSize * 0.6f;
                Rectangle bodyRect = {centerX - bodyWidth/2, centerY - bodyHeight/2, bodyWidth, bodyHeight};

                // Draw main body
                DrawRectangleRounded(bodyRect, 0.8, 6, bodyColor);
                DrawRectangleRoundedLinesEx(bodyRect, 0.8, 6, 2.0f, BLACK);

                // DRAGON ARMS WITH CLAWS - Extending from sides
                float armLength = cellSize * 0.4f;
                float armWidth = 3.0f;
                Color armColor = scaleColor;
                Color clawColor = {255, 215, 0, 255}; // Golden claws

                // Left arm
                Vector2 leftArmStart = {centerX - bodyWidth/2, centerY};
                Vector2 leftArmEnd = {centerX - bodyWidth/2 - armLength, centerY - armLength/2};
                DrawLineEx(leftArmStart, leftArmEnd, armWidth, armColor);

                // Left claws (3 small lines)
                float clawLength = cellSize * 0.15f;
                DrawLineEx(leftArmEnd, {leftArmEnd.x - clawLength, leftArmEnd.y - clawLength/2}, 2, clawColor);
                DrawLineEx(leftArmEnd, {leftArmEnd.x - clawLength, leftArmEnd.y}, 2, clawColor);
                DrawLineEx(leftArmEnd, {leftArmEnd.x - clawLength, leftArmEnd.y + clawLength/2}, 2, clawColor);

                // Right arm
                Vector2 rightArmStart = {centerX + bodyWidth/2, centerY};
                Vector2 rightArmEnd = {centerX + bodyWidth/2 + armLength, centerY - armLength/2};
                DrawLineEx(rightArmStart, rightArmEnd, armWidth, armColor);

                // Right claws (3 small lines)
                DrawLineEx(rightArmEnd, {rightArmEnd.x + clawLength, rightArmEnd.y - clawLength/2}, 2, clawColor);
                DrawLineEx(rightArmEnd, {rightArmEnd.x + clawLength, rightArmEnd.y}, 2, clawColor);
                DrawLineEx(rightArmEnd, {rightArmEnd.x + clawLength, rightArmEnd.y + clawLength/2}, 2, clawColor);

                // Dragon scales (small decorative lines) with dragon-specific color
                if (i % 2 == 0) { // Every other segment
                    DrawLineEx({centerX - bodyWidth/4, centerY}, {centerX + bodyWidth/4, centerY}, 1, scaleColor);
                }
            } else {
                // REGULAR DRAGON BODY - Slimmer, more serpentine with dragon-specific colors
                float bodyWidth = cellSize * 0.6f; // Slimmer than head
                float bodyHeight = cellSize * 0.6f;
                Rectangle bodyRect = {centerX - bodyWidth/2, centerY - bodyHeight/2, bodyWidth, bodyHeight};

                DrawRectangleRounded(bodyRect, 0.8, 6, bodyColor);
                DrawRectangleRoundedLinesEx(bodyRect, 0.8, 6, 2.0f, BLACK);

                // Dragon scales (small decorative lines) with dragon-specific color
                if (i % 2 == 0) { // Every other segment
                    DrawLineEx({centerX - bodyWidth/4, centerY}, {centerX + bodyWidth/4, centerY}, 1, scaleColor);
                }
            }
        }
    }

    void Update()
    {
        if (direction.x == 0 && direction.y == 0) return; // Hard stop: don't move or shrink

        body.push_front(Vector2Add(body[0], direction));
        if (addSegment == true)
        {
            addSegment = false;
        }
        else if (segmentsToAdd > 0)
        {
            segmentsToAdd--;
        }
        else
        {
            body.pop_back();
        }
    }

    void AddSegments(int count)
    {
        segmentsToAdd += count;
    }

    void UpdateDirectionFromMouse(Vector2 mousePos)
    {
        // Get snake head position in screen coordinates
        Vector2 headScreenPos = {
            gameAreaOffset + body[0].x * cellSize + cellSize / 2,
            gameAreaOffset + body[0].y * cellSize + cellSize / 2
        };

        // Calculate direction vector from head to mouse
        Vector2 directionVector = Vector2Subtract(mousePos, headScreenPos);

        // Normalize to get the primary direction
        Vector2 newDirection = {0, 0};

        if (abs(directionVector.x) > abs(directionVector.y))
        {
            // Horizontal movement is dominant
            newDirection.x = (directionVector.x > 0) ? 1 : -1;
            newDirection.y = 0;
        }
        else
        {
            // Vertical movement is dominant
            newDirection.x = 0;
            newDirection.y = (directionVector.y > 0) ? 1 : -1;
        }

        // Prevent 180-degree turns
        if (!(newDirection.x == -direction.x && newDirection.y == -direction.y))
        {
            direction = newDirection;
        }
    }

    void UpdateDirectionFromGamepad(int gamepad)
    {
        Vector2 newDirection = {0, 0};
        float stickX = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        float stickY = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
        float threshold = 0.5f;

        if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) newDirection = {0, -1};
        else if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) newDirection = {0, 1};
        else if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) newDirection = {-1, 0};
        else if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) newDirection = {1, 0};
        else if (abs(stickX) > threshold || abs(stickY) > threshold) {
            if (abs(stickX) > abs(stickY)) {
                newDirection.x = (stickX > 0) ? 1 : -1;
            } else {
                newDirection.y = (stickY > 0) ? 1 : -1;
            }
        }

        if ((newDirection.x != 0 || newDirection.y != 0) && 
            !(newDirection.x == -direction.x && newDirection.y == -direction.y))
        {
            direction = newDirection;
        }
    }

    void Reset()
    {
        body = {Vector2{10, 10}, Vector2{9, 10}, Vector2{8, 10}, Vector2{7, 10}};
        direction = {1, 0};
        segmentsToAdd = 0;
    }
};

class Food
{

public:
    Vector2 position;
    Texture2D texture;

    Food(deque<Vector2> snakeBody)
    {
        Image image = LoadImage("Graphics/food.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        position = GenerateRandomPos(snakeBody);
    }

    ~Food()
    {
        UnloadTexture(texture);
    }

    void Draw(const Tile& nextTile = Tile(1, PLAIN_TILES))
    {
        // MAHJONG TILE - Authentic Chinese game piece with correct type and color!
        float centerX = gameAreaOffset + position.x * cellSize + cellSize / 2;
        float centerY = gameAreaOffset + position.y * cellSize + cellSize / 2;
        float tileWidth = cellSize * 0.7f;
        float tileHeight = cellSize * 0.9f;

        // Mahjong tile colors - ivory white with type-specific border
        Color tileColor = {255, 255, 240, 255}; // Ivory white
        Color borderColor = {139, 69, 19, 255}; // Default saddle brown border
        Color shadowColor = {0, 0, 0, 100}; // Semi-transparent shadow
        Color symbolColor = {220, 20, 60, 255}; // Default crimson for symbols

        // Set colors based on tile type
        if (nextTile.type == HAT_TILES) {
            borderColor = {100, 200, 255, 255}; // Bright blue border for hat tiles
            symbolColor = {0, 100, 255, 255}; // Blue text for hat tiles
        } else if (nextTile.type == DOT_TILES) {
            borderColor = {100, 255, 100, 255}; // Bright green border for dot tiles
            symbolColor = {0, 150, 0, 255}; // Green text for dot tiles
        }

        // Draw shadow first (offset slightly)
        Rectangle shadowRect = {centerX - tileWidth/2 + 3, centerY - tileHeight/2 + 3, tileWidth, tileHeight};
        DrawRectangleRounded(shadowRect, 0.1, 6, shadowColor);

        // Draw main tile body
        Rectangle tileRect = {centerX - tileWidth/2, centerY - tileHeight/2, tileWidth, tileHeight};
        DrawRectangleRounded(tileRect, 0.1, 6, tileColor);
        DrawRectangleRoundedLinesEx(tileRect, 0.1, 6, 3.0f, borderColor); // Thicker border for type visibility

        // Draw Mahjong symbol with type indicator
        string tileText = to_string(nextTile.value);
        if (nextTile.type == HAT_TILES) {
            tileText += "^"; // Hat symbol
        } else if (nextTile.type == DOT_TILES) {
            tileText += "."; // Dot symbol
        }

        // Calculate text position for centering
        int fontSize = (int)(tileWidth * 0.5f);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), tileText.c_str(), fontSize, 1);
        float textX = centerX - textSize.x / 2;
        float textY = centerY - textSize.y / 2;

        // Draw the number with type symbol and shadow effect
        DrawTextEx(GetFontDefault(), tileText.c_str(), {textX + 1, textY + 1}, fontSize, 1, {0, 0, 0, 100}); // Shadow
        DrawTextEx(GetFontDefault(), tileText.c_str(), {textX, textY}, fontSize, 1, symbolColor); // Main text

        // Draw decorative elements based on tile type
        float dotRadius = 2.0f;
        if (nextTile.type == HAT_TILES) {
            // Blue dots for hat tiles
            Color dotColor = {0, 100, 255, 255};
            DrawCircle(centerX - tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX - tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
        } else if (nextTile.type == DOT_TILES) {
            // Green dots for dot tiles
            Color dotColor = {0, 150, 0, 255};
            DrawCircle(centerX - tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX - tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
        } else {
            // Traditional green dots for plain tiles
            Color dotColor = {34, 139, 34, 255};
            DrawCircle(centerX - tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY - tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX - tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
            DrawCircle(centerX + tileWidth/3, centerY + tileHeight/3, dotRadius, dotColor);
        }
    }

    Vector2 GenerateRandomCell()
    {
        // Avoid borders (2 cells from edge) for better gameplay
        float x = GetRandomValue(2, cellCount - 3);
        float y = GetRandomValue(2, cellCount - 3);
        return Vector2{x, y};
    }

    Vector2 GenerateRandomPos(deque<Vector2> snakeBody)
    {
        Vector2 position = GenerateRandomCell();
        while (ElementInDeque(position, snakeBody))
        {
            position = GenerateRandomCell();
        }
        return position;
    }
};

class Game
{
public:
    Snake snake = Snake();
    Food food = Food(snake.body);
    GameState gameState = TITLE_SCREEN;
    int score = 0;
    bool allowMove = false;
    Color green = {173, 204, 96, 255};
    Color darkGreen = {43, 51, 24, 255};
    Color lightGreen = {200, 230, 150, 255};
    Color currentBackgroundColor = {200, 230, 150, 255};
    int highScore = 0;

    // Input and Gamepad state
    float selectRepeatTimer = 0.0f;
    const float REPEAT_DELAY = 0.3f;
    const float REPEAT_RATE = 0.05f;
    bool stickUp = false, stickDown = false, stickLeft = false, stickRight = false;
    bool stickUpLast = false, stickDownLast = false, stickLeftLast = false, stickRightLast = false;

    // Difficulty and High Score System
    DifficultyLevel selectedDifficulty = FOUNDATION_BUILDING;
    map<pair<LoongType, DifficultyLevel>, int> loongHighScores; // [LoongType, Difficulty] -> High Score
    map<pair<LoongType, DifficultyLevel>, bool> difficultyUnlocked; // Track unlocked difficulties
    int mahjongWinsRequired = 0; // How many Mahjongs needed to win (for higher difficulties)
    int mahjongWinsAchieved = 0; // How many Mahjongs achieved this run

    // Difficulty system data
    vector<string> difficultyNames = {
        "Foundation Building",
        "Golden Core",
        "Nascent Soul",
        "Ascension",
        "Immortal Sage"
    };

    vector<string> difficultyDescriptions = {
        "Start with 3 Extra Lives",
        "Start with only 1 life, no Extra Lives",
        "Start with 7 tiles, need 2 Mahjongs to reach 13 tiles",
        "Start with 10 tiles, need 5 Mahjongs to win",
        "Start with 13 tiles, need 5 Mahjongs to win"
    };
    int mahjongWins = 0;
    int kongWins = 0;
    int ornateLevel = LEVEL_1_NONE;
    int totalWins = 0; // Track total KONG + Mahjong wins for tile expansion
    int currentTileCount = 4; // Starting with 4 tiles
    int waterHealingAmount = 0;
    int windLengthReduction = 0;
    Sound eatSound;
    Sound wallSound;
    Sound mahjongWinSound;

    // Sound performance optimization
    float lastSoundTime = 0.0f;
    float soundCooldown = 0.1f; // Minimum 100ms between sounds
    float lastMusicUpdateTime = 0.0f;
    float musicUpdateInterval = 0.016f; // Update music every 16ms (60fps)

    // LOONG images
    Texture2D loongImage; // Current LOONG image
    bool loongImageLoaded;
    Music backgroundMusic;
    Music titleScreenMusic;
    Music loongSelectMusic;
    Music loongThemeMusic; // Current LOONG-specific theme
    Music finalStretchMusic1; // blades_and_beats.mp3
    Music finalStretchMusic2; // blades_and_beats_cn.mp3
    Music gameOverMusic1; // First game over track
    Music gameOverMusic2; // Second game over track
    Music currentGameOverMusic; // Currently selected game over music
    MahjongTiles mahjongTiles;
    NumberPopup numberPopup;
    float mahjongWinTimer;
    bool showMahjongWin;
    int titleMenuSelection = 0; // 0=Start, 1=High Score, 2=Exit
    bool musicLoaded = false;
    float countdownTimer;
    int countdownNumber;
    int finalScore = 0; // Store final score for game over screen

    // Mouse hold timing for fine control
    float leftClickHoldTime = 0.0f;
    float rightClickHoldTime = 0.0f;
    const float HOLD_THRESHOLD = 0.3f; // 0.3 seconds to trigger hold

    // Age and Choice System
    int snakeAge = 0; // How many tiles consumed (Age)
    int snakeLength = 0; // Actual snake body length (Length)
    bool showChoiceWindow = false;
    float choiceWindowTimer = 0.0f;
    int selectedChoice = 0;
    vector<string> currentChoices;
    vector<string> currentChoiceDescriptions;
    bool showKongWin = false;
    float kongWinTimer = 0.0f;
    bool showPhoenixRebirth = false;
    float phoenixRebirthTimer = 0.0f;
    bool showCosmicWisdom = false;
    float cosmicWisdomTimer = 0.0f;

    // SHIFT Power System - Tile-based cooldown dragon abilities
    int shiftCooldownTiles = 0; // Tiles consumed since last use
    int shiftCooldownMax = 10; // Tiles needed to recharge
    bool shiftPowerReady = true;
    string shiftPowerName = "";
    string shiftPowerDescription = "";

    // New upgrade tile system
    int tilesConsumed = 0;
    int nextUpgradeTileAt = 10; // First upgrade at 10 tiles
    bool upgradeThresholds[4] = {false, false, false, false}; // 10, 33, 66, 100
    int upgradeThresholdValues[4] = {10, 33, 66, 100};

    // Power-up effects tracking
    bool slowGrowthActive = false;
    bool mahjongNerfActive = false;
    bool speedDemonActive = false;
    bool luckyNumbersActive = false;
    bool minimalistActive = false;
    bool turtleModeActive = false;
    bool gamblerActive = false;
    bool perfectionistActive = false;
    bool collectorActive = false;
    bool survivorActive = false;
    bool speedsterActive = false;
    bool monkActive = false;

    int extraLives = 0;
    int fruitCounter = 0; // For tracking fruit-based effects

    // Debug UI variables (disabled for release)
    bool showDebugUI = false; // Disabled for release
    float currentSpeedMultiplier = 1.0f;
    float currentProbabilityBonus = 0.0f;
    bool isInExtraLifeMode = false; // When using extra life
    int phoenixRebirthCharges = 0; // Phoenix Rebirth charges with bonus scoring
    string lastExtraLifeType = ""; // Track what type of extra life was used

    // Audio controls
    bool isMuted = false;
    float masterVolume = 0.5f;

    // UI improvements
    bool showBackButton = false; // Show back button in appropriate screens

    // Latent upgrade system
    map<LoongType, int> loongTotalScores; // Total scores across all runs for each loong
    map<LoongType, int> loongUpgradeLevel; // Upgrade level (0-5) for each loong
    vector<int> latentUpgradeThresholds = {250, 500, 1000, 2000, 4000}; // Score thresholds for latent upgrades
    int selectedLatentLevel = 0; // Currently selected latent cultivation level (0 = none, 1-5 = levels)

    // Latent cultivation upgrade spawning queue - SEPARATE from normal upgrade system
    vector<LoongType> pendingLatentUpgrades;
    float latentUpgradeSpawnTimer = 0.0f;
    float latentUpgradeSpawnDelay = 1.0f; // 1 second between spawns
    bool latentCultivationSpawned = false; // Track if we've spawned latent cultivation tiles yet

    // Separate latent cultivation tile system
    Vector2 latentUpgradeTilePosition;
    bool latentUpgradeSpawned = false;
    LoongType latentUpgradeTileType;

    // Music dictionary system
    Music alternateMusic;
    int lastOrnateLevel = -1; // Track ornate level changes
    map<int, string> musicMap; // Map ornate level to music file

    // LOONG System - AMBITIOUS UPGRADE SYSTEM!
    vector<LoongData> availableLOONGs;
    LoongType selectedLoongType;
    int selectedLoongIndex;
    int loongUpgradeSelection; // For upgrade screen
    vector<int> upgradeChoices; // 3 random upgrade choices
    bool showLoongUpgrade;

    // LOONG Effect Tracking Variables
    float mahjongScoreMultiplier; // Multiplier for Mahjong scores
    float kongScoreMultiplier; // Multiplier for KONG scores
    int wallImmunities; // Number of wall collision immunities
    int teleportCharges; // Number of teleport charges
    // REMOVED: Legacy auto-complete system
    int loongWinMultiplier; // LOONG win score multiplier
    bool phoenixRebirth; // Can revive with full score
    int phoenixCharges; // Number of phoenix revivals available
    bool celestialRebirth; // Can revive like basic LOONG
    int celestialCharges; // Number of celestial revivals available

    // Patience LOONG pause system
    bool isPaused; // Game is paused
    float pauseTimer; // How long pause lasts
    bool canChangeDirection; // Can change direction during pause

    // Bai Ban (White LOONG) zero tile system
    int purificationCounter; // Counter for automatic purification
    int purificationThreshold; // Tiles needed for purification (15->10->5)
    int divineShieldZeros; // How many zeros to remove on death (3->2->1)

    // Cross-LOONG color mixing system
    vector<LoongType> mixedLoongTypes; // LOONGs we've gained powers from
    Color originalBodyColor, originalScaleColor; // Original colors

    // Advanced LOONG Effects
    int shadowCloneCharges; // Number of shadow clone uses
    int earthquakeCounter; // Counter for earthquake auto-shuffle
    int earthquakeThreshold; // Fruits needed for earthquake
    int tornadoCounter; // Counter for tornado tile swaps
    int tornadoThreshold; // Fruits needed for tornado
    int healingCounter; // Counter for healing waters
    bool graniteWillActive; // Immunity to negative effects

    // NEW UPGRADE TILE SYSTEM
    Vector2 upgradeTilePosition; // Position of upgrade tile on screen
    bool upgradeSpawned; // Is there an upgrade tile on screen?
    LoongType upgradeTileType; // What type of LOONG upgrade this tile gives
    int tilesConsumedSinceUpgrade; // Track tiles consumed for upgrade spawning
    int nextUpgradeThreshold; // Next threshold for upgrade spawn (10, 33, 66, 100)
    bool isGraniteWillActive; // Proper Granite Will immunity state

    Game()
    {
        InitAudioDevice();
        eatSound = LoadSound("Sounds/eat.mp3");
        wallSound = LoadSound("Sounds/wall.mp3");
        mahjongWinSound = eatSound; // We'll modify pitch in PlaySound

        // Load title screen music
        titleScreenMusic = LoadMusicStream("Sounds/title_screen.mp3");
        if (titleScreenMusic.stream.buffer != NULL) {
            SetMusicVolume(titleScreenMusic, 0.5f);
            cout << "Successfully loaded title_screen.mp3" << endl;
        } else {
            cout << "Failed to load title_screen.mp3" << endl;
        }

        // Load LOONG selection music
        loongSelectMusic = LoadMusicStream("Sounds/select_loong.mp3");
        if (loongSelectMusic.stream.buffer != NULL) {
            SetMusicVolume(loongSelectMusic, masterVolume);
            cout << "Successfully loaded select_loong.mp3" << endl;
        } else {
            cout << "Failed to load select_loong.mp3" << endl;
        }

        // Initialize latent upgrade system
        InitializeLatentUpgrades();

        // Load final stretch music (random selection)
        finalStretchMusic1 = LoadMusicStream("Sounds/final_stretch/blades_and_beats.mp3");
        if (finalStretchMusic1.stream.buffer != NULL) {
            SetMusicVolume(finalStretchMusic1, 0.5f);
            cout << "Successfully loaded blades_and_beats.mp3" << endl;
        } else {
            cout << "Failed to load blades_and_beats.mp3" << endl;
        }

        finalStretchMusic2 = LoadMusicStream("Sounds/final_stretch/blades_and_beats_cn.mp3");
        if (finalStretchMusic2.stream.buffer != NULL) {
            SetMusicVolume(finalStretchMusic2, 0.5f);
            cout << "Successfully loaded blades_and_beats_cn.mp3" << endl;
        } else {
            cout << "Failed to load blades_and_beats_cn.mp3" << endl;
        }

        // Load game over music (random selection)
        gameOverMusic1 = LoadMusicStream("Sounds/game_over/game_over_eng.mp3");
        if (gameOverMusic1.stream.buffer != NULL) {
            SetMusicVolume(gameOverMusic1, 0.5f);
            cout << "Successfully loaded game_over_eng.mp3" << endl;
        } else {
            cout << "Failed to load game_over_eng.mp3" << endl;
        }

        gameOverMusic2 = LoadMusicStream("Sounds/game_over/game_over_cn.mp3");
        if (gameOverMusic2.stream.buffer != NULL) {
            SetMusicVolume(gameOverMusic2, 0.5f);
            cout << "Successfully loaded game_over_cn.mp3" << endl;
        } else {
            cout << "Failed to load game_over_cn.mp3" << endl;
        }

        // Initialize with no LOONG theme loaded yet
        loongThemeMusic = {0};
        backgroundMusic = {0}; // Will be set when LOONG is selected
        alternateMusic = {0}; // Will be set to final stretch music
        currentGameOverMusic = {0}; // Will be set when game over occurs

        musicLoaded = true;
        cout << "All music files loaded successfully!" << endl;
        cout << "Music system: Title -> LOONG Selection -> LOONG Theme (Levels 0-3) -> Final Stretch (Level 4+) -> Game Over" << endl;
        // Don't start music yet - wait for game to begin

        mahjongWinTimer = 0;
        showMahjongWin = false;
        currentBackgroundColor = {8, 35, 8, 255}; // Dark green for basic dragon background

        // Initialize LOONG System - GO WILD!
        InitializeLOONGSystem();
        selectedLoongType = BASIC_LOONG;
        selectedLoongIndex = 0;
        loongUpgradeSelection = 0;
        showLoongUpgrade = false;

        // Initialize Shift power system
        InitializeShiftPower();

        // Initialize LOONG effect variables
        mahjongScoreMultiplier = 1.0f;
        kongScoreMultiplier = 1.0f;
        wallImmunities = 0;
        teleportCharges = 0;
        // REMOVED: Legacy auto-complete system
        loongWinMultiplier = 3; // Default LOONG multiplier
        phoenixRebirth = false;
        phoenixCharges = 0;
        celestialRebirth = false;
        celestialCharges = 0;

        // Initialize pause system
        isPaused = false;
        pauseTimer = 0.0f;
        canChangeDirection = false;

        // Initialize zero tile system
        purificationCounter = 0;
        purificationThreshold = 15; // Default: 15 tiles
        divineShieldZeros = 3; // Default: remove 3 zeros on death

        // Initialize color mixing system
        mixedLoongTypes.clear();

        // Initialize LOONG image system
        loongImageLoaded = false;

        // Initialize advanced LOONG effects
        shadowCloneCharges = 0;
        earthquakeCounter = 0;
        earthquakeThreshold = 10; // Default: 10 fruits for earthquake
        tornadoCounter = 0;
        tornadoThreshold = 7; // Default: 7 fruits for tornado
        healingCounter = 0;
        graniteWillActive = false;

        // Initialize upgrade tile system
        upgradeSpawned = false;
        tilesConsumedSinceUpgrade = 0;
        nextUpgradeThreshold = 10; // First upgrade at 10 tiles
        isGraniteWillActive = false;

        LoadHighScore();
        InitializeDifficultySystem();
    }

    // Get dragon colors based on selected LOONG type
    void GetDragonColors(Color& bodyColor, Color& scaleColor) {
        if (selectedLoongIndex >= 0 && selectedLoongIndex < (int)availableLOONGs.size()) {
            LoongData& selectedLoong = availableLOONGs[selectedLoongIndex];

            // Start with original LOONG colors
            bodyColor = selectedLoong.primaryColor;
            scaleColor = selectedLoong.secondaryColor;

            // Apply color mixing if we have cross-LOONG powers
            if (!mixedLoongTypes.empty()) {
                cout << "🎨 MIXING COLORS: " << mixedLoongTypes.size() << " cross-LOONG powers!" << endl;

                // Calculate mixing ratios based on number of mixed LOONGs
                int totalLOONGs = 1 + mixedLoongTypes.size(); // Original + mixed
                float originalRatio = 1.0f / totalLOONGs;
                float mixedRatio = originalRatio; // Each mixed LOONG gets equal share

                // Start with original colors weighted by ratio
                int mixedBodyR = (int)(bodyColor.r * originalRatio);
                int mixedBodyG = (int)(bodyColor.g * originalRatio);
                int mixedBodyB = (int)(bodyColor.b * originalRatio);

                int mixedScaleR = (int)(scaleColor.r * originalRatio);
                int mixedScaleG = (int)(scaleColor.g * originalRatio);
                int mixedScaleB = (int)(scaleColor.b * originalRatio);

                // Add each mixed LOONG's colors
                for (LoongType mixedType : mixedLoongTypes) {
                    Color mixedBodyColor, mixedScaleColor;
                    GetOriginalLoongColors(mixedType, mixedBodyColor, mixedScaleColor);

                    mixedBodyR += (int)(mixedBodyColor.r * mixedRatio);
                    mixedBodyG += (int)(mixedBodyColor.g * mixedRatio);
                    mixedBodyB += (int)(mixedBodyColor.b * mixedRatio);

                    mixedScaleR += (int)(mixedScaleColor.r * mixedRatio);
                    mixedScaleG += (int)(mixedScaleColor.g * mixedRatio);
                    mixedScaleB += (int)(mixedScaleColor.b * mixedRatio);
                }

                // Apply mixed colors
                bodyColor = {(unsigned char)min(255, mixedBodyR),
                           (unsigned char)min(255, mixedBodyG),
                           (unsigned char)min(255, mixedBodyB), 255};
                scaleColor = {(unsigned char)min(255, mixedScaleR),
                            (unsigned char)min(255, mixedScaleG),
                            (unsigned char)min(255, mixedScaleB), 255};

                cout << "🎨 MIXED DRAGON: " << totalLOONGs << " LOONGs combined!" << endl;
            }

            // Make colors more vibrant and ensure they pop out against dark backgrounds
            bodyColor.r = min(255, bodyColor.r + 30);
            bodyColor.g = min(255, bodyColor.g + 30);
            bodyColor.b = min(255, bodyColor.b + 30);

            scaleColor.r = min(255, scaleColor.r + 20);
            scaleColor.g = min(255, scaleColor.g + 20);
            scaleColor.b = min(255, scaleColor.b + 20);
        } else {
            // Default to green (Bamboo Dragon colors)
            bodyColor = {34, 139, 34, 255};
            scaleColor = {144, 238, 144, 255};
        }
    }

    void LoadLoongImage() {
        // Unload previous LOONG image if loaded
        if (loongImageLoaded) {
            UnloadTexture(loongImage);
            loongImageLoaded = false;
        }

        // Load LOONG-specific image
        string imagePath = "";
        switch (selectedLoongType) {
            case BASIC_LOONG:
                imagePath = "Graphics/dragon_image/fa_cai.png";
                break;
            case FIRE_LOONG:
                imagePath = "Graphics/dragon_image/hong_zhong.png";
                break;
            case WATER_LOONG:
                imagePath = "Graphics/dragon_image/long_wang.png";
                break;
            case WHITE_LOONG:
                imagePath = "Graphics/dragon_image/bai_ban.png";
                break;
            case EARTH_LOONG:
                imagePath = "Graphics/dragon_image/huang_di.png";
                break;
            case WIND_LOONG:
                imagePath = "Graphics/dragon_image/qing_long.png";
                break;
            case SHADOW_LOONG:
                imagePath = "Graphics/dragon_image/ju_long.png";
                break;
            case CELESTIAL_LOONG:
                imagePath = "Graphics/dragon_image/shen_long.png";
                break;
            case PATIENCE_LOONG:
                imagePath = "Graphics/dragon_image/ren_long.png";
                break;
        }

        // LOONG image system shelved for future implementation
    }

    void GetOriginalLoongColors(LoongType loongType, Color& bodyColor, Color& scaleColor) {
        // Get the original colors for any LOONG type (for color mixing)
        switch (loongType) {
            case BASIC_LOONG:
                bodyColor = {34, 139, 34, 255}; scaleColor = {144, 238, 144, 255}; break;
            case FIRE_LOONG:
                bodyColor = {220, 20, 60, 255}; scaleColor = {255, 69, 0, 255}; break;
            case WATER_LOONG:
                bodyColor = {30, 144, 255, 255}; scaleColor = {173, 216, 230, 255}; break;
            case WHITE_LOONG:
                bodyColor = {248, 248, 255, 255}; scaleColor = {220, 220, 220, 255}; break;
            case EARTH_LOONG:
                bodyColor = {218, 165, 32, 255}; scaleColor = {255, 215, 0, 255}; break;
            case WIND_LOONG:
                bodyColor = {0, 191, 255, 255}; scaleColor = {135, 206, 250, 255}; break;
            case SHADOW_LOONG:
                bodyColor = {25, 25, 112, 255}; scaleColor = {72, 61, 139, 255}; break;
            case CELESTIAL_LOONG:
                bodyColor = {255, 215, 0, 255}; scaleColor = {100, 100, 100, 255}; break;
            case PATIENCE_LOONG:
                bodyColor = {138, 43, 226, 255}; scaleColor = {75, 0, 130, 255}; break;
            default:
                bodyColor = {34, 139, 34, 255}; scaleColor = {144, 238, 144, 255}; break;
        }
    }

    void InitializeLOONGSystem() {
        cout << ">>> INITIALIZING EPIC LOONG SYSTEM! <<<" << endl;

        // BASIC LOONG - Balanced Starter Dragon (Mahjong Green)
        LoongData basicLoong(BASIC_LOONG, "Fa Cai - Green Dragon", "Green Dragon of wealth\nand steady growth",
                            {34, 139, 34, 255}, {144, 238, 144, 255}); // Dark Green & Light Green (Mahjong colors)
        basicLoong.upgrades.push_back(LoongUpgrade("Mahjong Mastery", "Mahjong score +25% per level", 5));
        basicLoong.upgrades.push_back(LoongUpgrade("KONG Power", "KONG score +50% per level", 4));
        basicLoong.upgrades.push_back(LoongUpgrade("Extra Lives", "Gain +1 extra life per level", 3));
        basicLoong.upgrades.push_back(LoongUpgrade("Tile Wisdom", "SHIFT: Discard & change to next tile: 8->6->4 tiles", 3));
        basicLoong.upgrades.push_back(LoongUpgrade("Speed Control", "Reduce speed by 15% per level", 3));
        availableLOONGs.push_back(basicLoong);

        // FIRE LOONG - Aggressive Damage Dealer (Red Dragon)
        LoongData fireLoong(FIRE_LOONG, "Hong Zhong - Red Dragon", "Crimson Dragon of success\nand fiery determination",
                           {220, 20, 60, 255}, {255, 69, 0, 255}); // Deep Red & Orange Red
        fireLoong.upgrades.push_back(LoongUpgrade("Blazing Mahjong", "Mahjong +50% score +5% speed per level", 4));
        fireLoong.upgrades.push_back(LoongUpgrade("Inferno KONG", "KONG +75% score +10% speed per level", 3));
        fireLoong.upgrades.push_back(LoongUpgrade("Speed Demon", "+20% speed +20% score +1 extra life", 3));
        fireLoong.upgrades.push_back(LoongUpgrade("Burning Tiles", "SHIFT: Change to most held: 12->9->6 tiles", 3));
        fireLoong.upgrades.push_back(LoongUpgrade("Blazing Rebirth", "Phoenix rebirth +extra score: 1->2->3 charges", 3));
        availableLOONGs.push_back(fireLoong);

        // WATER LOONG - Defensive and Healing (Blue Dragon)
        LoongData waterLoong(WATER_LOONG, "Long Wang - Dragon King", "Azure Dragon of flowing\nwisdom and healing",
                            {30, 144, 255, 255}, {173, 216, 230, 255}); // Dodger Blue & Light Blue
        waterLoong.upgrades.push_back(LoongUpgrade("Flowing Mahjong", "Mahjong +20% score + heal 1 length per level", 5));
        waterLoong.upgrades.push_back(LoongUpgrade("Tidal KONG", "KONG +30% score + 2 extra lives per level", 3));
        waterLoong.upgrades.push_back(LoongUpgrade("Tsunami Shield", "Wall immunity +3 uses per level", 4));
        waterLoong.upgrades.push_back(LoongUpgrade("Healing Waters", "Reduce length by 2->4->6 every 5 fruits", 4));
        waterLoong.upgrades.push_back(LoongUpgrade("Tidal Wave", "SHIFT: Heal, Shuffle & Shield: 8 tiles", 3));
        availableLOONGs.push_back(waterLoong);

        // WHITE LOONG - Purity and Zero Dragon (Bai Ban - White Tile)
        LoongData whiteLoong(WHITE_LOONG, "Bai Ban - White Tile", "Pure dragon of emptiness\nand zero mastery",
                            {248, 248, 255, 255}, {220, 220, 220, 255}); // Ghost White & Light Gray
        whiteLoong.upgrades.push_back(LoongUpgrade("Pure Mahjong", "1->2->3 tiles become 0 after Mahjong", 3));
        whiteLoong.upgrades.push_back(LoongUpgrade("Sacred KONG", "KONG +40%, four 0 KONG +200%", 3));
        whiteLoong.upgrades.push_back(LoongUpgrade("Divine Shield", "Remove 3->2->1 zeros on death", 3));
        whiteLoong.upgrades.push_back(LoongUpgrade("Zero Mastery", "SHIFT: Turn next tile to 0: 10 tiles", 3));
        whiteLoong.upgrades.push_back(LoongUpgrade("Purification", "Turn leftmost tile to 0: 15->10->5", 3));
        availableLOONGs.push_back(whiteLoong);

        // EARTH LOONG - Slow but Powerful (Yellow Dragon)
        LoongData earthLoong(EARTH_LOONG, "Huang Di - Yellow Emperor", "Golden Emperor Dragon\nof earth and stability",
                            {218, 165, 32, 255}, {255, 215, 0, 255}); // Golden Rod & Gold
        earthLoong.upgrades.push_back(LoongUpgrade("Mountain Mahjong", "Mahjong +60% score -25% KONG score per level", 3));
        earthLoong.upgrades.push_back(LoongUpgrade("Boulder KONG", "KONG +100% score -25% Mahjong score per level", 2));
        earthLoong.upgrades.push_back(LoongUpgrade("Stone Skin", "-15% speed +3 lives -15% score on death", 3));
        earthLoong.upgrades.push_back(LoongUpgrade("Earthquake", "KONG/consecutive probability +20% per level", 4));
        earthLoong.upgrades.push_back(LoongUpgrade("Granite Will", "SHIFT: Wall immunity 20-30 hits: 15 tiles", 5));
        availableLOONGs.push_back(earthLoong);

        // WIND LOONG - Fast and Agile (Azure Dragon)
        LoongData windLoong(WIND_LOONG, "Qing Long - Azure Dragon", "Blue Dragon of the East\nand swift winds",
                           {0, 191, 255, 255}, {135, 206, 250, 255}); // Deep Sky Blue & Light Sky Blue
        windLoong.upgrades.push_back(LoongUpgrade("Gale Mahjong", "Mahjong +60% score +25% speed per level", 5));
        windLoong.upgrades.push_back(LoongUpgrade("Whirlwind KONG", "KONG +25% score + probability boost per level", 4));
        windLoong.upgrades.push_back(LoongUpgrade("Lightning Speed", "+25% speed reduce length 1->2->3 every 5 tiles", 3));
        windLoong.upgrades.push_back(LoongUpgrade("Tornado Tiles", "SHIFT: Replace pointed tile: 6 tiles", 4));
        windLoong.upgrades.push_back(LoongUpgrade("Wind Walk", "Phase through self +3 times per level", 3));
        availableLOONGs.push_back(windLoong);

        // SHADOW LOONG - Mysterious and Tricky (Black Dragon)
        LoongData shadowLoong(SHADOW_LOONG, "Ju Long - Night Dragon", "Shadow Dragon of mystery\nand dimensional power",
                             {25, 25, 112, 255}, {72, 61, 139, 255}); // Midnight Blue & Dark Slate Blue
        shadowLoong.upgrades.push_back(LoongUpgrade("Void Mahjong", "Mahjong +35% score hide 3->6->9 tiles from pool", 3));
        shadowLoong.upgrades.push_back(LoongUpgrade("Dark KONG", "KONG +80% score discard 1->2->3 tiles on KONG", 3));
        shadowLoong.upgrades.push_back(LoongUpgrade("Shadow Clone", "Duplicate Mahjong win once per level", 2));
        shadowLoong.upgrades.push_back(LoongUpgrade("Nightmare Tiles", "SHIFT: Force next tile same: 8->6->4 tiles", 3));
        shadowLoong.upgrades.push_back(LoongUpgrade("Dimensional Rift", "Teleport on collision +3 charges per level", 3));
        availableLOONGs.push_back(shadowLoong);

        // CELESTIAL LOONG - Ultimate Endgame Dragon (Imperial Dragon)
        LoongData celestialLoong(CELESTIAL_LOONG, "Shen Long - Divine Dragon", "Divine Imperial Dragon\nof ultimate power",
                                {255, 215, 0, 255}, {100, 100, 100, 255}); // Gold & Dark Gray (better readability)
        celestialLoong.upgrades.push_back(LoongUpgrade("Divine Mahjong", "Mahjong +50% score per level", 3));
        celestialLoong.upgrades.push_back(LoongUpgrade("Heavenly KONG", "KONG +75% score per level", 3));
        celestialLoong.upgrades.push_back(LoongUpgrade("Celestial Rebirth", "+2 celestial rebirths per level", 3));
        celestialLoong.upgrades.push_back(LoongUpgrade("Cosmic Wisdom", "SHIFT: Instant MAHJONG redraw: 20->15->10 tiles", 3));
        celestialLoong.upgrades.push_back(LoongUpgrade("Transcendence", "LOONG multiplier: 4x->5x->6x", 3));
        availableLOONGs.push_back(celestialLoong);

        // PATIENCE LOONG - Dragon of Patience and Control
        LoongData patienceLoong(PATIENCE_LOONG, "Ren Long - Patience Dragon", "Serene dragon of patience\nand perfect timing",
                               {138, 43, 226, 255}, {75, 0, 130, 255}); // Blue Violet & Indigo
        patienceLoong.upgrades.push_back(LoongUpgrade("Calm Mahjong", "+30% score, -10% speed per level", 3));
        patienceLoong.upgrades.push_back(LoongUpgrade("Steady KONG", "+35% score, immunity on KONG", 3));
        patienceLoong.upgrades.push_back(LoongUpgrade("Meditation", "+2 wall immunity per level", 3));
        patienceLoong.upgrades.push_back(LoongUpgrade("Pause Control", "SHIFT: Pause & change direction: 3->2->1 tiles", 4));
        patienceLoong.upgrades.push_back(LoongUpgrade("Patience Mastery", "Slower = higher score multiplier", 3));
        availableLOONGs.push_back(patienceLoong);

        cout << "*** LOONG SYSTEM INITIALIZED WITH " << availableLOONGs.size() << " MYTHOLOGICAL DRAGONS! ***" << endl;
        cout << ">>> BAMBOO -> RED -> WHITE -> YELLOW -> AZURE -> SHADOW -> CELESTIAL <<<" << endl;
        cout << "*** Each dragon has 5 unique upgrade paths rooted in Chinese mythology! ***" << endl;
    }

    void InitializeShiftPower() {
        // Initialize Shift power based on selected LOONG type
        switch (selectedLoongType) {
            case BASIC_LOONG:
                shiftPowerName = "Tile Wisdom";
                shiftPowerDescription = "Discard & change to next";
                shiftCooldownMax = 8; // 8 tiles cooldown
                break;

            case FIRE_LOONG:
                shiftPowerName = "Burning Tiles";
                shiftPowerDescription = "Change tile to most held";
                shiftCooldownMax = 12; // 12 tiles cooldown
                break;

            case WATER_LOONG:
                shiftPowerName = "Tidal Wave";
                shiftPowerDescription = "Heal snake + shuffle tiles";
                shiftCooldownMax = 8; // 8 tiles cooldown
                break;

            case WHITE_LOONG:
                shiftPowerName = "Zero Mastery";
                shiftPowerDescription = "Turn next tile to 0";
                shiftCooldownMax = 10; // Reduced from 15
                break;

            case EARTH_LOONG:
                shiftPowerName = "Granite Will";
                shiftPowerDescription = "Temporary immunity";
                shiftCooldownMax = 15; // 15 tiles cooldown
                break;

            case WIND_LOONG:
                shiftPowerName = "Tornado Tiles";
                shiftPowerDescription = "Replace pointed tile";
                shiftCooldownMax = 6; // 6 tiles cooldown
                break;

            case SHADOW_LOONG:
                shiftPowerName = "Nightmare Tiles";
                shiftPowerDescription = "Change to pointed tile";
                shiftCooldownMax = 8; // 8 tiles cooldown
                break;

            case CELESTIAL_LOONG:
                shiftPowerName = "Cosmic Wisdom";
                shiftPowerDescription = "Instant MAHJONG";
                shiftCooldownMax = 20; // 20 tiles cooldown
                break;

            case PATIENCE_LOONG:
                shiftPowerName = "Pause Control";
                shiftPowerDescription = "Pause & change direction";
                shiftCooldownMax = 3; // 3 tiles cooldown (very low)
                break;
        }

        // Start with power ready
        shiftPowerReady = true;
        shiftCooldownTiles = 0;

        cout << "*** SHIFT POWER INITIALIZED: " << shiftPowerName << " ***" << endl;
    }

    void LoadLoongThemeMusic() {
        // Check if we already have the correct theme loaded
        static LoongType lastLoadedLoongType = (LoongType)-1;
        if (lastLoadedLoongType == selectedLoongType && loongThemeMusic.stream.buffer != NULL) {
            return; // Already loaded correct theme
        }

        // Unload previous LOONG theme if loaded
        if (loongThemeMusic.stream.buffer != NULL) {
            UnloadMusicStream(loongThemeMusic);
        }

        lastLoadedLoongType = selectedLoongType;

        // Load LOONG-specific theme music
        string themePath = "";
        switch (selectedLoongType) {
            case BASIC_LOONG:
                themePath = "Sounds/loong_theme/fa_cai.mp3";
                break;
            case FIRE_LOONG:
                themePath = "Sounds/loong_theme/hong_zhong.mp3";
                break;
            case WATER_LOONG:
                themePath = "Sounds/loong_theme/long_wang.mp3";
                break;
            case WHITE_LOONG:
                themePath = "Sounds/loong_theme/bai_ban.mp3";
                break;
            case EARTH_LOONG:
                themePath = "Sounds/loong_theme/huang_di.mp3";
                break;
            case WIND_LOONG:
                themePath = "Sounds/loong_theme/qing_long.mp3";
                break;
            case SHADOW_LOONG:
                themePath = "Sounds/loong_theme/ju_long.mp3";
                break;
            case CELESTIAL_LOONG:
                themePath = "Sounds/loong_theme/shen_long.mp3";
                break;
            case PATIENCE_LOONG:
                themePath = "Sounds/loong_theme/ren_long.mp3";
                break;
        }

        if (!themePath.empty()) {
            loongThemeMusic = LoadMusicStream(themePath.c_str());
            if (loongThemeMusic.stream.buffer != NULL) {
                SetMusicVolume(loongThemeMusic, 0.5f);
                cout << "Successfully loaded LOONG theme: " << themePath << endl;

                // Set this as the background music for ornate levels 0-2
                backgroundMusic = loongThemeMusic;
            } else {
                cout << "Failed to load LOONG theme: " << themePath << endl;
            }
        }

        // Set up final stretch music (random selection)
        if (GetRandomValue(0, 1) == 0) {
            alternateMusic = finalStretchMusic1;
            cout << "Selected blades_and_beats.mp3 for final stretch" << endl;
        } else {
            alternateMusic = finalStretchMusic2;
            cout << "Selected blades_and_beats_cn.mp3 for final stretch" << endl;
        }
    }

    void ActivateShiftPower() {
        if (!shiftPowerReady) return;

        cout << "*** ACTIVATING SHIFT POWER: " << shiftPowerName << " ***" << endl;

        switch (selectedLoongType) {
            case BASIC_LOONG: {
                // Tile Wisdom: Discard current tile and change to next one
                Tile discardedTile = mahjongTiles.nextTile;
                mahjongTiles.AddToDiscardPile(discardedTile);
                mahjongTiles.GenerateNextTile(currentProbabilityBonus);
                cout << ">>> TILE WISDOM: Discarded " << discardedTile.ToString() << " and generated new tile!" << endl;
                break;
            }
            case FIRE_LOONG: {
                // Burning Tiles: Change tile to most held (non-KONG) tile
                map<pair<int, TileType>, int> heldCounts;
                for (const Tile& tile : mahjongTiles.tiles) {
                    pair<int, TileType> key = {tile.value, tile.type};
                    if (mahjongTiles.goldTiles.count(key) == 0) { // Not KONG
                        heldCounts[key]++;
                    }
                }

                if (!heldCounts.empty()) {
                    auto mostHeld = max_element(heldCounts.begin(), heldCounts.end(),
                        [](const auto& a, const auto& b) { return a.second < b.second; });

                    // Discard the original tile and generate the new one
                    Tile originalTile = mahjongTiles.nextTile;
                    mahjongTiles.AddToDiscardPile(originalTile);
                    mahjongTiles.nextTile = Tile(mostHeld->first.first, mostHeld->first.second, false);
                    cout << ">>> BURNING TILES: Discarded " << originalTile.ToString() << ", changed to " << mahjongTiles.nextTile.ToString() << " (most held)!" << endl;
                } else {
                    cout << ">>> BURNING TILES: No non-KONG tiles to copy!" << endl;
                }
                break;
            }

            case WATER_LOONG: {
                // Tidal Wave: Heal snake and shuffle tiles
                cout << ">>> TIDAL WAVE: Healing and reshuffling!" << endl;

                // Heal snake by reducing length (water healing)
                int healAmount = min(3, (int)snake.body.size() - 4); // Don't go below 4 segments
                for (int i = 0; i < healAmount; i++) {
                    if (snake.body.size() > 4) {
                        snake.body.pop_back();
                    }
                }
                cout << ">>> HEALING WATERS: Reduced length by " << healAmount << " segments!" << endl;

                // Shuffle tiles (tidal wave effect)
                mahjongTiles.ReshuffleTiles();
                cout << ">>> TIDAL SHUFFLE: Tiles reshuffled by the wave!" << endl;

                // Grant temporary immunity (water protection)
                wallImmunities += 2;
                cout << ">>> WATER SHIELD: Gained 2 immunity charges!" << endl;
                break;
            }

            case WHITE_LOONG: {
                // Zero Mastery: Turn next tile into 0 (blank)
                cout << ">>> ZERO MASTERY: Turning next tile into blank!" << endl;

                // Discard the original tile and create a zero tile
                Tile originalTile = mahjongTiles.nextTile;
                mahjongTiles.AddToDiscardPile(originalTile);
                mahjongTiles.nextTile = Tile(0, PLAIN_TILES, false); // Create zero tile
                cout << ">>> ZERO MASTERY: Discarded " << originalTile.ToString() << ", next tile is now 0 (blank)!" << endl;

                // Grant temporary immunity (White LOONG purity power)
                wallImmunities += 1;
                cout << ">>> DIVINE PURITY: Gained 1 immunity charge! (" << wallImmunities << " total)" << endl;
                break;
            }

            case EARTH_LOONG: {
                // Granite Will: Temporary immunity to walls and own body (20-30 tiles duration)
                isGraniteWillActive = true; // Activate proper immunity mode
                wallImmunities += GetRandomValue(20, 30); // Much longer immunity duration
                cout << ">>> GRANITE WILL: Extended immunity activated! (" << wallImmunities << " hits)" << endl;
                break;
            }

            case WIND_LOONG: {
                // Tornado Tiles: Replace pointed tile with current tile (or discard if KEEP)
                if (mahjongTiles.IsKeepSelected()) {
                    // If pointing to KEEP, just discard the current tile
                    Tile discardedTile = mahjongTiles.nextTile;
                    mahjongTiles.AddToDiscardPile(discardedTile);
                    mahjongTiles.GenerateNextTile(currentProbabilityBonus);
                    cout << ">>> TORNADO TILES: Discarded " << discardedTile.ToString() << " (KEEP selected)!" << endl;
                } else {
                    // Replace the pointed tile with current tile
                    int arrowPos = mahjongTiles.arrowPosition;
                    if (arrowPos >= 0 && arrowPos < (int)mahjongTiles.tiles.size()) {
                        Tile replacedTile = mahjongTiles.tiles[arrowPos];
                        mahjongTiles.AddToDiscardPile(replacedTile); // Properly discard the replaced tile
                        mahjongTiles.tiles[arrowPos] = mahjongTiles.nextTile;
                        mahjongTiles.SortTiles();
                        mahjongTiles.GenerateNextTile(currentProbabilityBonus);

                        // Add snake growth for using the ability (wind power gives energy)
                        snake.body.push_back(snake.body.back());
                        cout << ">>> TORNADO TILES: Replaced " << replacedTile.ToString() << " with current tile! Snake grew!" << endl;
                    }
                }
                break;
            }

            case SHADOW_LOONG: {
                // Nightmare Tiles: Change current tile to whatever tile we are pointing to
                if (mahjongTiles.IsKeepSelected()) {
                    cout << ">>> NIGHTMARE TILES: Cannot activate when pointing to KEEP!" << endl;
                } else {
                    int arrowPos = mahjongTiles.arrowPosition;
                    if (arrowPos >= 0 && arrowPos < (int)mahjongTiles.tiles.size()) {
                        Tile pointedTile = mahjongTiles.tiles[arrowPos];

                        // Check if it's a KONG tile
                        pair<int, TileType> key = {pointedTile.value, pointedTile.type};
                        if (mahjongTiles.goldTiles.count(key) > 0) {
                            cout << ">>> NIGHTMARE TILES: Cannot activate on KONG tile!" << endl;
                        } else {
                            Tile originalTile = mahjongTiles.nextTile;
                            mahjongTiles.AddToDiscardPile(originalTile);
                            mahjongTiles.nextTile = Tile(pointedTile.value, pointedTile.type, false);
                            cout << ">>> NIGHTMARE TILES: Changed " << originalTile.ToString() << " to " << mahjongTiles.nextTile.ToString() << "!" << endl;
                        }
                    }
                }
                break;
            }

            case CELESTIAL_LOONG: {
                // Cosmic Wisdom: Instantly MAHJONG and redraw hand
                cout << ">>> COSMIC WISDOM: Instant MAHJONG and hand redraw!" << endl;

                // Apply instant Mahjong bonus
                mahjongWins++;
                mahjongWinsAchieved++; // Track for difficulty system
                ornateLevel = min(mahjongWins, 3);

                int cosmicPoints = mahjongTiles.tiles.size() * 10; // Higher bonus for instant
                score += cosmicPoints;
                cout << ">>> COSMIC MAHJONG: " << cosmicPoints << " points!" << endl;

                // Add proper snake growth for instant Mahjong (like normal Mahjong wins)
                int growthAmount = mahjongTiles.tiles.size(); // Grow by number of tiles used
                for (int i = 0; i < growthAmount; i++) {
                    snake.body.push_back(snake.body.back()); // Add segments
                }
                cout << ">>> COSMIC GROWTH: Snake grew by " << growthAmount << " segments!" << endl;

                // Redraw hand completely
                ExpandTiles();
                mahjongTiles.GenerateNextTile(currentProbabilityBonus);

                // Show cosmic effect
                showCosmicWisdom = true;
                cosmicWisdomTimer = 3.0f;
                break;
            }

            case PATIENCE_LOONG: {
                // Pause Control: Pause game and allow direction change
                cout << ">>> PAUSE CONTROL: Time stops, choose new direction!" << endl;

                // Pause the game temporarily
                isPaused = true;
                pauseTimer = 2.0f; // 2 seconds to choose direction

                // Allow direction change during pause
                canChangeDirection = true;

                // Grant temporary immunity during pause
                wallImmunities += 1;
                cout << ">>> PATIENCE POWER: Game paused for 2 seconds! Choose direction with WASD!" << endl;
                break;
            }

            default:
                cout << "*** No special power for this LOONG type!" << endl;
                return;
        }

        // Start cooldown
        shiftPowerReady = false;
        shiftCooldownTiles = 0;
        cout << "*** SHIFT POWER ON COOLDOWN: " << shiftCooldownMax << " tiles ***" << endl;
    }

    void GenerateLoongUpgradeChoices() {
        upgradeChoices.clear();
        LoongData& currentLoong = availableLOONGs[selectedLoongIndex];

        // Find upgrades that can still be leveled
        vector<int> availableUpgrades;
        for (int i = 0; i < (int)currentLoong.upgrades.size(); i++) {
            if (currentLoong.upgrades[i].level < currentLoong.upgrades[i].maxLevel) {
                availableUpgrades.push_back(i);
            }
        }

        // If we have upgrades available, pick 3 random ones
        if (availableUpgrades.size() > 0) {
            for (int i = 0; i < 3 && availableUpgrades.size() > 0; i++) {
                int randomIndex = GetRandomValue(0, availableUpgrades.size() - 1);
                upgradeChoices.push_back(availableUpgrades[randomIndex]);
                availableUpgrades.erase(availableUpgrades.begin() + randomIndex);
            }
        }

        // Fill remaining slots with random upgrades if needed
        while (upgradeChoices.size() < 3) {
            int randomUpgrade = GetRandomValue(0, currentLoong.upgrades.size() - 1);
            upgradeChoices.push_back(randomUpgrade);
        }

        loongUpgradeSelection = 0;
        showLoongUpgrade = true;
        cout << "🎯 Generated " << upgradeChoices.size() << " upgrade choices for " << currentLoong.name << "!" << endl;
    }

    void ApplyLoongUpgrade(int upgradeIndex) {
        if (upgradeIndex < 0 || upgradeIndex >= (int)upgradeChoices.size()) return;

        LoongData& currentLoong = availableLOONGs[selectedLoongIndex];
        int actualUpgradeIndex = upgradeChoices[upgradeIndex];
        LoongUpgrade& upgrade = currentLoong.upgrades[actualUpgradeIndex];

        if (upgrade.level < upgrade.maxLevel) {
            upgrade.level++;
            currentLoong.level++;

            cout << "*** UPGRADED " << upgrade.name << " to level " << upgrade.level << "!" << endl;
            cout << ">>> " << currentLoong.name << " is now level " << currentLoong.level << "!" << endl;

            // Apply upgrade effects (this is where the magic happens!)
            ApplyUpgradeEffects(currentLoong.type, actualUpgradeIndex, upgrade.level);

            // CROSS-LOONG COLOR MIXING: Add this LOONG to mixed types if it's different
            if (currentLoong.type != selectedLoongType) {
                // Check if we already have this LOONG type in our mix
                bool alreadyMixed = false;
                for (LoongType mixedType : mixedLoongTypes) {
                    if (mixedType == currentLoong.type) {
                        alreadyMixed = true;
                        break;
                    }
                }

                if (!alreadyMixed) {
                    mixedLoongTypes.push_back(currentLoong.type);
                    cout << "🎨 DRAGON FUSION! Added " << currentLoong.name << " to color mix!" << endl;
                    cout << "🎨 Total mixed LOONGs: " << mixedLoongTypes.size() << " + original = " << (mixedLoongTypes.size() + 1) << " dragons!" << endl;
                }
            }
        }

        showLoongUpgrade = false;
    }

    void ApplyUpgradeEffects(LoongType loongType, int upgradeIndex, int level) {
        cout << "*** APPLYING UPGRADE EFFECTS! ***" << endl;

        // This is where each LOONG's unique abilities get applied!
        // For now, we'll implement basic effects and expand later
        switch (loongType) {
            case BASIC_LOONG:
                ApplyBasicLoongUpgrade(upgradeIndex, level);
                break;
            case FIRE_LOONG:
                ApplyFireLoongUpgrade(upgradeIndex, level);
                break;
            case WATER_LOONG:
                ApplyWaterLoongUpgrade(upgradeIndex, level);
                break;
            case WHITE_LOONG:
                ApplyWhiteLoongUpgrade(upgradeIndex, level);
                break;
            case EARTH_LOONG:
                ApplyEarthLoongUpgrade(upgradeIndex, level);
                break;
            case WIND_LOONG:
                ApplyWindLoongUpgrade(upgradeIndex, level);
                break;
            case SHADOW_LOONG:
                ApplyShadowLoongUpgrade(upgradeIndex, level);
                break;
            case CELESTIAL_LOONG:
                ApplyCelestialLoongUpgrade(upgradeIndex, level);
                break;
            case PATIENCE_LOONG:
                ApplyPatienceLoongUpgrade(upgradeIndex, level);
                break;
        }
    }

    // BASIC LOONG Upgrade Effects - Balanced and Reliable
    void ApplyBasicLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Mahjong Mastery - +25% per level
                mahjongScoreMultiplier += 0.25f;
                cout << "🎯 Mahjong Mastery Level " << level << ": Mahjong multiplier now " << mahjongScoreMultiplier << "x!" << endl;
                break;
            case 1: // KONG Power - +50% per level
                kongScoreMultiplier += 0.5f;
                cout << "💪 KONG Power Level " << level << ": KONG multiplier now " << kongScoreMultiplier << "x!" << endl;
                break;
            case 2: // Extra Lives - +1 basic extra life per level
                extraLives += 1;
                cout << "❤️ Basic Extra Lives Level " << level << ": Gained 1 basic extra life! Total: " << extraLives << endl;
                break;
            case 3: // Tile Wisdom - Reduce SHIFT cooldown
                // Reduce SHIFT cooldown: 10->8->6 seconds
                if (selectedLoongType == BASIC_LOONG) {
                    shiftCooldownMax = 10.0f - (level * 2.0f); // 10, 8, 6 seconds
                    cout << "🧠 Tile Wisdom Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << "s!" << endl;
                }
                break;
            case 4: // Speed Control - -15% speed per level
                currentSpeedMultiplier *= 0.85f;
                cout << "🐌 Speed Control Level " << level << ": Speed reduced to " << (currentSpeedMultiplier * 100) << "%!" << endl;
                break;
        }
    }

    // FIRE LOONG Upgrade Effects - Aggressive and Powerful
    void ApplyFireLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Blazing Mahjong - +40% score +5% speed per level
                mahjongScoreMultiplier += 0.5f;
                currentSpeedMultiplier *= 1.05f;
                cout << ">>> Blazing Mahjong Level " << level << ": Mahjong " << mahjongScoreMultiplier << "x, speed " << (currentSpeedMultiplier * 100) << "%!" << endl;
                break;
            case 1: // Inferno KONG - +75% score +10% speed per level
                kongScoreMultiplier += 0.75f;
                currentSpeedMultiplier *= 1.1f;
                cout << ">>> Inferno KONG Level " << level << ": KONG " << kongScoreMultiplier << "x, speed " << (currentSpeedMultiplier * 100) << "%!" << endl;
                break;
            case 2: // Speed Demon - +20% speed +20% score +1 extra life
                currentSpeedMultiplier *= 1.2f;
                mahjongScoreMultiplier += 0.2f;
                kongScoreMultiplier += 0.2f;
                phoenixRebirthCharges += 1;
                cout << ">>> Speed Demon Level " << level << ": Speed " << (currentSpeedMultiplier * 100) << "%, +20% all scores, +1 Phoenix Rebirth!" << endl;
                break;
            case 3: // Burning Tiles - Reduce SHIFT cooldown: 15->12->9s
                if (selectedLoongType == FIRE_LOONG) {
                    shiftCooldownMax = 15.0f - (level * 3.0f);
                    cout << ">>> Burning Tiles Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << "s!" << endl;
                }
                break;
            case 4: // Blazing Rebirth - Phoenix rebirth +extra score
                phoenixRebirth = true;
                phoenixCharges += 1;
                phoenixRebirthCharges += 1; // Grant Phoenix Rebirth with bonus score on revival
                cout << ">>> Blazing Rebirth Level " << level << ": Phoenix rebirth " << phoenixCharges << " times with extra score!" << endl;
                break;
        }
    }

    // WATER LOONG Upgrade Effects - Defensive and Healing
    void ApplyWaterLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Flowing Mahjong - +20% + heal
                mahjongScoreMultiplier += 0.2f;
                if (snake.body.size() > 4) snake.body.pop_back(); // Heal by reducing length
                cout << "🌊 Flowing Mahjong Level " << level << ": Mahjong multiplier " << mahjongScoreMultiplier << "x + healed 1 length!" << endl;
                break;
            case 1: // Tidal KONG - +30% + 2 special extra lives
                kongScoreMultiplier += 0.3f;
                phoenixRebirthCharges += 2;
                cout << ">>> Tidal KONG Level " << level << ": KONG multiplier " << kongScoreMultiplier << "x + 2 Phoenix Rebirths!" << endl;
                break;
            case 2: // Tsunami Shield - Wall immunity +3 per level
                wallImmunities += 3;
                cout << ">>> Tsunami Shield Level " << level << ": " << wallImmunities << " wall collision immunities!" << endl;
                break;
            case 3: // Healing Waters - Reduce length
                waterHealingAmount = 2 * level;
                cout << ">>> Healing Waters Level " << level << ": Reduce length by " << waterHealingAmount << " every 5 fruits!" << endl;
                break;
            case 4: // Ocean Wisdom - SHIFT power (see next 3 tiles)
                // This is now a SHIFT power, no passive effect
                cout << ">>> Ocean Wisdom Level " << level << ": SHIFT power enhanced!" << endl;
                break;
        }
    }

    // WHITE LOONG Upgrade Effects - Zero Mastery and Purity
    void ApplyWhiteLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Pure Mahjong - 1->2->3 tiles become 0 after Mahjong
                // This will be handled in the Mahjong win logic
                cout << ">>> Pure Mahjong Level " << level << ": " << level << " tiles become 0 after Mahjong!" << endl;
                break;
            case 1: // Sacred KONG - +40% KONG score, +200% for four 0 KONG
                kongScoreMultiplier += 0.4f;
                cout << ">>> Sacred KONG Level " << level << ": KONG " << kongScoreMultiplier << "x, four 0 KONG +200%!" << endl;
                break;
            case 2: // Divine Shield - Remove 3->2->1 zeros on death
                divineShieldZeros = max(1, 4 - level); // 3, 2, 1 zeros removed
                cout << ">>> Divine Shield Level " << level << ": Remove " << divineShieldZeros << " zeros on death!" << endl;
                break;
            case 3: // Zero Mastery - SHIFT power enhanced (cooldown reduction)
                if (selectedLoongType == WHITE_LOONG) {
                    shiftCooldownMax = max(2, 10 - (level * 2)); // 8, 6, 4 tiles
                    cout << ">>> Zero Mastery Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << " tiles!" << endl;
                }
                break;
            case 4: // Purification - Turn leftmost tile to 0: 15->10->5 tiles
                purificationThreshold = max(5, 20 - (level * 5)); // 15, 10, 5 tiles
                cout << ">>> Purification Level " << level << ": Auto-purify leftmost tile every " << purificationThreshold << " tiles!" << endl;
                break;
        }
    }

    // EARTH LOONG Upgrade Effects - Slow but Mighty
    void ApplyEarthLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Mountain Mahjong - +60% Mahjong score -25% KONG score
                mahjongScoreMultiplier += 0.6f;
                kongScoreMultiplier -= 0.25f;
                cout << "⛰️ Mountain Mahjong Level " << level << ": Mahjong " << mahjongScoreMultiplier << "x, KONG " << kongScoreMultiplier << "x!" << endl;
                break;
            case 1: // Boulder KONG - +100% KONG score -25% Mahjong score
                kongScoreMultiplier += 1.0f;
                mahjongScoreMultiplier -= 0.25f;
                cout << ">>> Boulder KONG Level " << level << ": KONG " << kongScoreMultiplier << "x, Mahjong " << mahjongScoreMultiplier << "x!" << endl;
                break;
            case 2: // Stone Skin - -15% speed +3 lives -15% score on death
                currentSpeedMultiplier *= 0.85f;
                extraLives += 3;
                cout << ">>> Stone Skin Level " << level << ": Speed " << (currentSpeedMultiplier * 100) << "%, +3 lives, -15% score on death!" << endl;
                break;
            case 3: // Earthquake - KONG/consecutive probability +20%
                currentProbabilityBonus += 20.0f;
                cout << ">>> Earthquake Level " << level << ": +" << (level * 20) << "% KONG/consecutive probability!" << endl;
                break;
            case 4: // Granite Will - SHIFT power (temporary immunity)
                graniteWillActive = true;
                isGraniteWillActive = true; // Activate proper Granite Will immunity
                wallImmunities += GetRandomValue(15 + level * 5, 25 + level * 5); // Scaling immunity duration
                cout << ">>> Granite Will Level " << level << ": SHIFT power enhanced! (" << wallImmunities << " charges)" << endl;
                break;
        }
    }

    // WIND LOONG Upgrade Effects - Fast and Agile
    void ApplyWindLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Gale Mahjong - +50% score +25% speed (buffed for speed penalty)
                mahjongScoreMultiplier += 0.6f;
                currentSpeedMultiplier *= 1.25f;
                cout << ">>> Gale Mahjong Level " << level << ": Mahjong " << mahjongScoreMultiplier << "x + Speed " << (currentSpeedMultiplier * 100) << "%!" << endl;
                break;
            case 1: // Whirlwind KONG - +25% + probability boost
                kongScoreMultiplier += 0.25f;
                currentProbabilityBonus += 15.0f;
                cout << ">>> Whirlwind KONG Level " << level << ": KONG " << kongScoreMultiplier << "x + probability boost!" << endl;
                break;
            case 2: // Lightning Speed - +25% speed, reduce length 1->2->3 every 5 tiles
                currentSpeedMultiplier *= 1.25f;
                windLengthReduction = level;
                cout << ">>> Lightning Speed Level " << level << ": Speed " << (currentSpeedMultiplier * 100) << "%, reduce length " << windLengthReduction << " every 5 tiles!" << endl;
                break;
            case 3: // Tornado Tiles - SHIFT power (instantly get tile)
                // This is now a SHIFT power
                cout << ">>> Tornado Tiles Level " << level << ": SHIFT power enhanced!" << endl;
                break;
            case 4: // Wind Walk - Phase through self +3 times per level
                wallImmunities += 3; // Use wall immunities for self-phasing too
                cout << ">>> Wind Walk Level " << level << ": Phase through self " << (level * 3) << " times!" << endl;
                break;
        }
    }

    // SHADOW LOONG Upgrade Effects - Mysterious and Tricky (REBALANCED FOR SINGLE PLAYER)
    void ApplyShadowLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Void Mahjong - +35% score, hide 3->6->9 tiles from pool
                mahjongScoreMultiplier += 0.35f;
                cout << ">>> Void Mahjong Level " << level << ": Mahjong " << mahjongScoreMultiplier << "x, hide " << (level * 3) << " tiles from pool!" << endl;
                break;
            case 1: // Dark KONG - +80% score, discard 1->2->3 tiles on KONG (high risk/reward)
                kongScoreMultiplier += 0.8f;
                cout << ">>> Dark KONG Level " << level << ": KONG " << kongScoreMultiplier << "x, discard " << level << " tiles on KONG!" << endl;
                break;
            case 2: // Shadow Clone - Duplicate Mahjong win once per level
                shadowCloneCharges += 1;
                cout << ">>> Shadow Clone Level " << level << ": " << shadowCloneCharges << " shadow clone charges (once per level)!" << endl;
                break;
            case 3: // Nightmare Tiles - SHIFT power (force next tile same): 10->8->6s
                if (selectedLoongType == SHADOW_LOONG) {
                    shiftCooldownMax = 10.0f - (level * 2.0f);
                    cout << ">>> Nightmare Tiles Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << "s!" << endl;
                }
                break;
            case 4: // Dimensional Rift - Teleport on collision +3 charges per level
                teleportCharges += 3;
                cout << ">>> Dimensional Rift Level " << level << ": " << teleportCharges << " total teleport charges!" << endl;
                break;
        }
    }

    // CELESTIAL LOONG Upgrade Effects - Ultimate Divine Power (REBALANCED)
    void ApplyCelestialLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Divine Mahjong - +50% per level (rebalanced from +100%)
                mahjongScoreMultiplier += 0.5f;
                cout << ">>> Divine Mahjong Level " << level << ": Mahjong multiplier now " << mahjongScoreMultiplier << "x!" << endl;
                break;
            case 1: // Heavenly KONG - +75% per level (rebalanced from +200%)
                kongScoreMultiplier += 0.75f;
                cout << ">>> Heavenly KONG Level " << level << ": KONG multiplier now " << kongScoreMultiplier << "x!" << endl;
                break;
            case 2: // Celestial Rebirth - +2 celestial rebirths (normal revival like Bamboo)
                celestialRebirth = true;
                celestialCharges += 2;
                cout << ">>> Celestial Rebirth Level " << level << ": Gained 2 Celestial Rebirths! Total: " << celestialCharges << endl;
                break;
            case 3: // Cosmic Wisdom - SHIFT power (instant MAHJONG): 30s cooldown
                if (selectedLoongType == CELESTIAL_LOONG) {
                shiftCooldownMax = max(10, 25 - (level * 5)); // Scales down: 20 -> 15 -> 10 tiles
                cout << ">>> Cosmic Wisdom Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << " tiles!" << endl;
                }
                break;
            case 4: // Transcendence - LOONG multiplier (4x->5x->6x)
                loongWinMultiplier = 3 + level; // 4x, 5x, 6x
                cout << ">>> Transcendence Level " << level << ": LOONG win multiplier is now " << loongWinMultiplier << "x!" << endl;
                break;
        }
    }

    // PATIENCE LOONG Upgrade Effects - Dragon of Patience and Control
    void ApplyPatienceLoongUpgrade(int upgradeIndex, int level) {
        switch (upgradeIndex) {
            case 0: // Calm Mahjong - +30% score, -10% speed per level
                mahjongScoreMultiplier += 0.3f;
                currentSpeedMultiplier *= 0.9f; // 10% slower
                cout << ">>> Calm Mahjong Level " << level << ": Mahjong " << mahjongScoreMultiplier << "x, speed " << (currentSpeedMultiplier * 100) << "%!" << endl;
                break;
            case 1: // Steady KONG - +35% score, immunity on KONG
                kongScoreMultiplier += 0.35f;
                wallImmunities += 1; // Gain immunity on each KONG
                cout << ">>> Steady KONG Level " << level << ": KONG " << kongScoreMultiplier << "x, +1 immunity per KONG!" << endl;
                break;
            case 2: // Meditation - +2 wall immunity per level
                wallImmunities += 2;
                cout << ">>> Meditation Level " << level << ": +" << (level * 2) << " wall immunities! Total: " << wallImmunities << endl;
                break;
            case 3: // Pause Control - SHIFT power cooldown reduction: 3->2->1->0
                if (selectedLoongType == PATIENCE_LOONG) {
                    shiftCooldownMax = max(0.0f, 3.0f - level); // 3, 2, 1, 0 tiles
                    cout << ">>> Pause Control Level " << level << ": SHIFT cooldown reduced to " << shiftCooldownMax << " tiles!" << endl;
                }
                break;
            case 4: // Patience Mastery - Slower = higher score multiplier
                // The slower you are, the higher your score multiplier
                float speedPenalty = 1.0f / currentSpeedMultiplier; // Inverse of speed
                mahjongScoreMultiplier += speedPenalty * 0.1f; // 10% bonus per speed reduction
                kongScoreMultiplier += speedPenalty * 0.1f;
                cout << ">>> Patience Mastery Level " << level << ": Score multiplier increased based on patience! Mahjong: " << mahjongScoreMultiplier << "x, KONG: " << kongScoreMultiplier << "x!" << endl;
                break;
        }
    }

    void DrawLoongImageInGameplay() {
        if (!loongImageLoaded || gameState != PLAYING) {
            return; // Only draw during gameplay
        }

        // Position in the right UI panel
        int uiPanelX = canvasWidth - uiPanelWidth;
        int panelPadding = 10;
        int availableWidth = uiPanelWidth - (panelPadding * 2);
        int availableHeight = 400; // Reserve space for UI elements

        // Scale image to fit within UI panel while maintaining aspect ratio
        float scaleX = (float)availableWidth / (float)loongImage.width;
        float scaleY = (float)availableHeight / (float)loongImage.height;
        float scale = fmin(scaleX, scaleY);

        // Make scale larger for better visibility
        scale = fmin(scale, 0.4f);

        int scaledWidth = (int)(loongImage.width * scale);
        int scaledHeight = (int)(loongImage.height * scale);

        // Position in right panel, below the UI elements
        int imageX = uiPanelX + panelPadding + (availableWidth - scaledWidth) / 2;
        int imageY = 400; // Higher up for better visibility

        // Make sure image fits in panel
        if (imageY + scaledHeight > canvasHeight - panelPadding) {
            imageY = canvasHeight - scaledHeight - panelPadding;
        }

        // Get current LOONG colors
        Color bodyColor, scaleColor;
        GetDragonColors(bodyColor, scaleColor);

        // Draw a visible test rectangle first to see positioning
        DrawRectangle(imageX - 5, imageY - 5, scaledWidth + 10, scaledHeight + 10, RED);
        DrawRectangle(imageX, imageY, scaledWidth, scaledHeight, {10, 10, 10, 255});

        // Use simpler texture drawing approach for better visibility
        Rectangle sourceRect = {0, 0, (float)loongImage.width, (float)loongImage.height};
        Rectangle destRect = {(float)imageX, (float)imageY, (float)scaledWidth, (float)scaledHeight};
        Vector2 origin = {0, 0};

        // Draw with dragon color tinting
        DrawTexturePro(loongImage, sourceRect, destRect, origin, 0.0f, bodyColor);
    }

    void DrawLoongSelectionScreen() {
        // Draw epic background
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {0, 0, 20, 255}); // Dark blue background

        // Draw title with golden glow effect
        int titleWidth = MeasureText("SELECT YOUR LOONG", 60);
        DrawText("SELECT YOUR LOONG", canvasWidth/2 - titleWidth/2 + 2, 52, 60, {255, 215, 0, 100}); // Gold shadow
        DrawText("SELECT YOUR LOONG", canvasWidth/2 - titleWidth/2, 50, 60, GOLD);

        // Draw subtitle
        int subtitleWidth = MeasureText("Choose your dragon companion for the journey ahead", 24);
        DrawText("Choose your dragon companion for the journey ahead",
                canvasWidth/2 - subtitleWidth/2, 120, 24, WHITE);

        // Draw LOONG cards in a grid (3x3 layout)
        int cardWidth = 350;
        int cardHeight = 200;
        int startX = (canvasWidth - (3 * cardWidth + 2 * 50)) / 2; // Center 3 cards with 50px spacing
        int startY = 180;

        for (int i = 0; i < (int)availableLOONGs.size(); i++) {
            int row = i / 3;
            int col = i % 3;
            int cardX = startX + col * (cardWidth + 50);
            int cardY = startY + row * (cardHeight + 30);

            LoongData& loong = availableLOONGs[i];

            // Check if this dragon is unlocked
            pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)i, FOUNDATION_BUILDING};
            bool isUnlocked = difficultyUnlocked[dragonKey];

            // Card background with LOONG colors
            Rectangle cardRect = {(float)cardX, (float)cardY, (float)cardWidth, (float)cardHeight};
            Color bgColor = loong.primaryColor;
            if (!isUnlocked) {
                bgColor = DARKGRAY; // Locked dragons are gray
                bgColor.a = 100;
            } else {
                bgColor.a = (i == selectedLoongIndex) ? 255 : 150; // Highlight selected
            }

            DrawRectangleRec(cardRect, bgColor);
            DrawRectangleLinesEx(cardRect, (i == selectedLoongIndex) ? 5 : 3,
                               (i == selectedLoongIndex) ? GOLD : (isUnlocked ? loong.secondaryColor : GRAY));

            // LOONG name
            int nameWidth = MeasureText(loong.name.c_str(), 28);
            Color nameColor = isUnlocked ? WHITE : GRAY;
            DrawText(loong.name.c_str(), cardX + cardWidth/2 - nameWidth/2, cardY + 10, 28, nameColor);

            if (!isUnlocked) {
                // Show lock status
                DrawText("LOCKED", cardX + cardWidth/2 - 40, cardY + 50, 24, RED);
                DrawText("Reach Cultivation Level 4", cardX + 10, cardY + 80, 16, GRAY);
                DrawText("with previous dragon", cardX + 10, cardY + 100, 16, GRAY);
            } else {
                // LOONG description (handle two lines)
                string desc = loong.description;
                size_t newlinePos = desc.find('\n');
                if (newlinePos != string::npos) {
                    string line1 = desc.substr(0, newlinePos);
                    string line2 = desc.substr(newlinePos + 1);
                    DrawText(line1.c_str(), cardX + 10, cardY + 50, 16, WHITE);
                    DrawText(line2.c_str(), cardX + 10, cardY + 70, 16, WHITE);
                } else {
                    DrawText(desc.c_str(), cardX + 10, cardY + 50, 16, WHITE);
                }

                // Selection indicator
                if (i == selectedLoongIndex) {
                    DrawText(">>> SELECTED <<<", cardX + cardWidth/2 - 80, cardY + cardHeight - 25, 16, GOLD);
                }
            }
        }

        // Instructions
        int instY = canvasHeight - 100;
        DrawText("Use ARROW KEYS or MOUSE to select your LOONG", canvasWidth/2 - 200, instY, 18, WHITE);
        DrawText("Press ENTER or LEFT CLICK to confirm selection", canvasWidth/2 - 180, instY + 30, 18, YELLOW);
        DrawText("Each LOONG has unique abilities and upgrade paths!", canvasWidth/2 - 200, instY + 60, 16, LIGHTGRAY);
    }

    void DrawInstructionScreen() {
        // Clean dark background
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {15, 15, 15, 255});

        LoongData& selectedLoong = availableLOONGs[selectedLoongIndex];

        // Two-column layout for better space utilization
        int leftCol = 300; // Move right from far left
        int rightCol = 600; // Move right for better balance
        int centerX = canvasWidth / 2;
        int startY = 50;

        // LOONG name - large and prominent at top center
        int nameWidth = MeasureText(selectedLoong.name.c_str(), 42);
        DrawText(selectedLoong.name.c_str(), centerX - nameWidth/2, startY, 42, GOLD);

        // LOONG description - compact
        string desc = selectedLoong.description;
        size_t newlinePos = desc.find('\n');
        if (newlinePos != string::npos) {
            string line1 = desc.substr(0, newlinePos);
            string line2 = desc.substr(newlinePos + 1);
            int line1Width = MeasureText(line1.c_str(), 20);
            int line2Width = MeasureText(line2.c_str(), 20);
            DrawText(line1.c_str(), centerX - line1Width/2, startY + 50, 20, WHITE);
            DrawText(line2.c_str(), centerX - line2Width/2, startY + 75, 20, WHITE);
        }

        // SHIFT ability - compact
        char shiftText[150];
        sprintf(shiftText, "SHIFT: %s", shiftPowerName.c_str());
        int shiftWidth = MeasureText(shiftText, 22);
        DrawText(shiftText, centerX - shiftWidth/2, startY + 105, 22, GOLD);

        // LEFT COLUMN - Controls
        int leftY = startY + 140;
        DrawText("CONTROLS", 150, leftY, 28, GOLD);
        leftY += 40;

        DrawText("• Mouse: Move snake", 150, leftY, 18, WHITE);
        leftY += 25;
        DrawText("• Wheel/Click: Select tiles", 150, leftY, 18, WHITE);
        leftY += 25;
        DrawText("• SHIFT: Dragon power", 150, leftY, 18, YELLOW);
        leftY += 25;
        DrawText("• SPACE: Reshuffle tiles", 150, leftY, 18, SKYBLUE);
        leftY += 35;

        // Game Rules
        DrawText("GAME RULES", 150, leftY, 28, GOLD);
        leftY += 40;

        DrawText("• Tiles drawn from limited pool", 150, leftY, 16, WHITE);
        leftY += 22;
        DrawText("• Groups: 3 same or consecutive", 150, leftY, 16, WHITE);
        leftY += 22;
        DrawText("• Pairs: 2 identical tiles", 150, leftY, 16, WHITE);
        leftY += 22;
        DrawText("• KONG: 4 same tiles (bonus)", 150, leftY, 16, GREEN);
        leftY += 22;
        DrawText("• SPACE: Shuffle when pool empty", 150, leftY, 16, SKYBLUE);
        leftY += 22;
        DrawText("• SHIFT: Unique dragon abilities", 150, leftY, 16, YELLOW);

        // RIGHT COLUMN - Win Conditions
        int rightY = startY + 140;
        DrawText("WIN CONDITIONS", 550, rightY, 28, GOLD);
        rightY += 40;

        DrawText("MAHJONG: 3 groups + 1 pair", 550, rightY, 18, WHITE);
        rightY += 25;
        DrawText("Example: 111 222 333 44", 550, rightY, 16, LIGHTGRAY);
        rightY += 30;

        DrawText("LOONG: 1-9 consecutive (3x score!)", 550, rightY, 18, GOLD);
        rightY += 25;
        DrawText("Example: 123456789", 550, rightY, 16, LIGHTGRAY);
        rightY += 30;

        // Difficulty info
        DrawText("DIFFICULTY RULES", 550, rightY, 28, GOLD);
        rightY += 40;

        char diffText[100];
        sprintf(diffText, "Level: %s", difficultyNames[selectedDifficulty].c_str());
        DrawText(diffText, 550, rightY, 18, YELLOW);
        rightY += 25;

        if (mahjongWinsRequired > 0) {
            char reqText[100];
            sprintf(reqText, "Need %d Mahjongs before LOONG", mahjongWinsRequired);
            DrawText(reqText, 550, rightY, 16, RED);
            rightY += 25;
        }

        DrawText("Level 4: KEEP = LOONG only!", 550, rightY, 16, RED);

        // Ready prompt - prominent at bottom center
        int readyY = canvasHeight - 80;
        int readyWidth = MeasureText("CLICK TO START!", 36);
        DrawText("CLICK TO START!", centerX - readyWidth/2, readyY, 36, GREEN);
    }

    void DrawLoongUpgradeScreen() {
        // Semi-transparent overlay
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {0, 0, 0, 180});

        LoongData& currentLoong = availableLOONGs[selectedLoongIndex];

        // Main upgrade window
        int windowWidth = 800;
        int windowHeight = 600;
        int windowX = canvasWidth/2 - windowWidth/2;
        int windowY = canvasHeight/2 - windowHeight/2;

        Rectangle windowBg = {(float)windowX, (float)windowY, (float)windowWidth, (float)windowHeight};
        DrawRectangleRec(windowBg, currentLoong.primaryColor);
        DrawRectangleLinesEx(windowBg, 5, GOLD);

        // Title with LOONG name - larger font
        char titleText[100];
        sprintf(titleText, "%s EVOLUTION", currentLoong.name.c_str());
        int titleWidth = MeasureText(titleText, 44);
        DrawText(titleText, canvasWidth/2 - titleWidth/2, windowY + 20, 44, GOLD);

        // Current level info - larger font
        char levelText[100];
        sprintf(levelText, "Choose your next evolution:");
        int levelWidth = MeasureText(levelText, 28);
        DrawText(levelText, canvasWidth/2 - levelWidth/2, windowY + 80, 28, WHITE);

        // Draw 3 upgrade choices
        for (int i = 0; i < (int)upgradeChoices.size() && i < 3; i++) {
            int choiceY = windowY + 130 + i * 120;
            int upgradeIndex = upgradeChoices[i];
            LoongUpgrade& upgrade = currentLoong.upgrades[upgradeIndex];

            // Choice background
            Color choiceBg = (i == loongUpgradeSelection) ? currentLoong.secondaryColor : BLACK;
            choiceBg.a = (i == loongUpgradeSelection) ? 200 : 100;
            Rectangle choiceRect = {(float)(windowX + 20), (float)(choiceY - 10), (float)(windowWidth - 40), 100.0f};
            DrawRectangleRec(choiceRect, choiceBg);
            DrawRectangleLinesEx(choiceRect, (i == loongUpgradeSelection) ? 4 : 2,
                               (i == loongUpgradeSelection) ? GOLD : WHITE);

            // Selection arrow
            if (i == loongUpgradeSelection) {
                DrawTriangle(
                    Vector2{(float)(windowX + 40), (float)(choiceY + 35)}, // Left point
                    Vector2{(float)(windowX + 55), (float)(choiceY + 25)}, // Top right
                    Vector2{(float)(windowX + 55), (float)(choiceY + 45)}, // Bottom right
                    GOLD
                );
            }

            // Upgrade name and level - larger font
            char upgradeTitle[100];
            sprintf(upgradeTitle, "%s (Level %d -> %d)",
                   upgrade.name.c_str(), upgrade.level,
                   min(upgrade.level + 1, upgrade.maxLevel));
            DrawText(upgradeTitle, windowX + 70, choiceY, 26, WHITE);

            // Upgrade description - larger font
            DrawText(upgrade.description.c_str(), windowX + 70, choiceY + 30, 18, LIGHTGRAY);

            // Max level indicator
            if (upgrade.level >= upgrade.maxLevel) {
                DrawText("MAX LEVEL REACHED", windowX + 70, choiceY + 55, 14, RED);
            } else {
                char progressText[50];
                sprintf(progressText, "Progress: %d/%d", upgrade.level, upgrade.maxLevel);
                DrawText(progressText, windowX + 70, choiceY + 55, 14, YELLOW);
            }
        }

        // Instructions - larger font
        DrawText("Use ARROW KEYS, MOUSE WHEEL, or MOUSE HOVER to select upgrade",
                canvasWidth/2 - 280, windowY + windowHeight - 80, 20, WHITE);
        DrawText("Press ENTER or LEFT CLICK to confirm upgrade",
                canvasWidth/2 - 200, windowY + windowHeight - 50, 20, YELLOW);

        // Show all current upgrades on the right side
        int rightPanelX = windowX + windowWidth + 20;
        DrawText("Current Upgrades:", rightPanelX, windowY + 20, 20, WHITE);

        for (int i = 0; i < (int)currentLoong.upgrades.size(); i++) {
            LoongUpgrade& upgrade = currentLoong.upgrades[i];
            char upgradeInfo[100];
            sprintf(upgradeInfo, "%s: %d/%d", upgrade.name.c_str(), upgrade.level, upgrade.maxLevel);

            Color upgradeColor = WHITE;
            if (upgrade.level == upgrade.maxLevel) upgradeColor = GOLD;
            else if (upgrade.level > 0) upgradeColor = LIGHTGREEN;

            DrawText(upgradeInfo, rightPanelX, windowY + 50 + i * 25, 14, upgradeColor);
        }
    }

    void GenerateChoices() {
        currentChoices.clear();
        currentChoiceDescriptions.clear();

        // Create a pool of REBALANCED choices with proper pros and cons
        vector<pair<string, string>> choicePool = {
            {"Slow Growth", "PRO: Only gain 1 length every 2 fruits\nCON: Slightly increase speed (10%)"},
            {"Length Reduction", "PRO: Remove 3 length from snake\nCON: Mahjong gives 20% less points"},
            {"Speed Demon", "PRO: Extra life (survive 1 collision)\nCON: Increase speed by 50%"},
            {"Lucky Numbers", "PRO: Current tiles 30% more likely\nCON: Snake grows faster (every 4 fruits)"},
            {"Minimalist", "PRO: Remove 5 length from snake\nCON: Tile pool depletes 25% faster"},
            {"Turtle Mode", "PRO: Reduce speed by 50%\nCON: Snake grows every 3 fruits"},
            {"Gambler", "PRO: Double points from Mahjong/Kong\nCON: Lose 1 length every 10 fruits"},
            {"Perfectionist", "PRO: Mahjong gives 50% bonus points\nCON: Speed increases 20% after Mahjong"},
            {"Collector", "PRO: Kong appears 40% more often\nCON: Snake grows 2 per fruit"},
            {"Survivor", "PRO: Extra life + 30% slower speed\nCON: All scores reduced by 30%"},
            {"Speedster", "PRO: Triple points from all sources\nCON: Speed increases by 200%"},
            {"Monk", "PRO: Remove 7 length + 40% slower\nCON: No bonus points from wins"}
        };

        // Randomly select 3 choices
        for (int i = 0; i < 3; i++) {
            int randomIndex = GetRandomValue(0, choicePool.size() - 1);
            currentChoices.push_back(choicePool[randomIndex].first);
            currentChoiceDescriptions.push_back(choicePool[randomIndex].second);
            choicePool.erase(choicePool.begin() + randomIndex);
        }

        selectedChoice = 0;
        showChoiceWindow = true;
        choiceWindowTimer = 0.0f; // No timer - let player take their time
    }

    void ApplyChoice(int choiceIndex) {
        if (choiceIndex < 0 || static_cast<size_t>(choiceIndex) >= currentChoices.size()) return;

        string choice = currentChoices[choiceIndex];

        // Check for Granite Will resistance to negative effects
        if (graniteWillActive) {
            // 25% chance per level to resist negative effects
            int resistanceChance = GetRandomValue(1, 100);
            if (resistanceChance <= 25) { // Simplified to 25% for now
                cout << "💎 GRANITE WILL ACTIVATED! Resisting negative effect: " << choice << endl;
                return; // Skip applying the negative effect
            }
        }

        cout << "Applied choice: " << choice << endl;

        // Apply the effects based on choice name
        if (choice == "Slow Growth") {
            slowGrowthActive = true;
            cout << "Slow Growth activated: Only gain 1 length every 2 fruit" << endl;
        }
        else if (choice == "Mahjong Nerf") {
            mahjongNerfActive = true;
            if (snake.body.size() > 7) { // Keep minimum size
                for (int i = 0; i < 3; i++) {
                    snake.body.pop_back();
                }
            }
            cout << "Mahjong Nerf activated: Removed 3 length, Mahjong no longer adds length" << endl;
        }
        else if (choice == "Speed Demon") {
            speedDemonActive = true;
            extraLives += 1; // Add to existing extra lives
            currentSpeedMultiplier *= 1.5f; // Increase speed by 50% (was incorrectly setting to 2.0f)
            cout << "Speed Demon activated: Extra life gained, speed increased by 50%" << endl;
        }
        else if (choice == "Lucky Numbers") {
            luckyNumbersActive = true;
            currentProbabilityBonus = 30.0f; // 30% bonus chance
            cout << "Lucky Numbers activated: Current tiles 30% more likely to appear" << endl;
        }
        else if (choice == "Minimalist") {
            minimalistActive = true;
            if (snake.body.size() > 9) { // Keep minimum size
                for (int i = 0; i < 5; i++) {
                    snake.body.pop_back();
                }
            }
            cout << "Minimalist activated: Removed 5 length, numbers less likely" << endl;
        }
        else if (choice == "Turtle Mode") {
            turtleModeActive = true;
            currentSpeedMultiplier *= 0.75f; // FIXED: Multiplicative (25% reduction)
            cout << "Turtle Mode activated: Speed reduced by 25%, every 3 fruits add 1 length" << endl;
        }
        else if (choice == "Gambler") {
            gamblerActive = true;
            cout << "Gambler activated: Double points from Mahjong/Kong, lose 2 length every 7 fruits" << endl;
        }
        else if (choice == "Perfectionist") {
            perfectionistActive = true;
            cout << "Perfectionist activated: Mahjong adds 8 length instead of 5" << endl;
        }
        else if (choice == "Collector") {
            collectorActive = true;
            cout << "Collector activated: Kong more frequent, snake grows 2 per fruit" << endl;
        }
        else if (choice == "Survivor") {
            survivorActive = true;
            extraLives += 1;
            currentSpeedMultiplier *= 0.85f; // FIXED: Multiplicative (15% reduction)
            cout << "Survivor activated: Extra life + 15% slower speed, no Mahjong/Kong bonuses" << endl;
        }
        else if (choice == "Speedster") {
            speedsterActive = true;
            currentSpeedMultiplier *= 1.5f; // FIXED: Multiplicative (50% increase)
            cout << "Speedster activated: Triple points from all sources, speed increased by 50%" << endl;
        }
        else if (choice == "Monk") {
            monkActive = true;
            if (snake.body.size() > 11) { // Keep minimum size
                for (int i = 0; i < 7; i++) {
                    snake.body.pop_back();
                }
            }
            currentSpeedMultiplier *= 0.8f; // FIXED: Multiplicative (20% reduction)
            cout << "Monk activated: Removed 7 length + 20% slower speed, no Mahjong/Kong bonuses" << endl;
        }

        cout << "Choice applied successfully!" << endl;
    }

    void ExpandTiles() {
        cout << "ExpandTiles called! Current tile count: " << currentTileCount << endl;
        cout << "Current mahjongTiles.maxTiles: " << mahjongTiles.maxTiles << endl;
        cout << "Current mahjongTiles.tiles.size(): " << mahjongTiles.tiles.size() << endl;

        // COMPLETE HAND REDRAW: 4 -> 7 -> 10 -> 13 (every win)
        if (currentTileCount == 4) {
            currentTileCount = 7;
            mahjongTiles.RedrawCompleteHand(7, ornateLevel);
            cout << "🎴 COMPLETE HAND REDRAW: 4 -> 7 tiles!" << endl;
        } else if (currentTileCount == 7) {
            currentTileCount = 10;
            mahjongTiles.RedrawCompleteHand(10, ornateLevel);
            cout << "🎴 COMPLETE HAND REDRAW: 7 -> 10 tiles!" << endl;
        } else if (currentTileCount == 10) {
            currentTileCount = 13;
            mahjongTiles.RedrawCompleteHand(13, ornateLevel);
            cout << "🎴 COMPLETE HAND REDRAW: 10 -> 13 tiles!" << endl;
        } else {
            // At maximum tiles (13) - still redraw for shuffling effect
            mahjongTiles.RedrawCompleteHand(currentTileCount, ornateLevel);
            cout << "🎴 COMPLETE HAND REDRAW: " << currentTileCount << " tiles (shuffle at max)!" << endl;
        }

        cout << "After redraw - currentTileCount: " << currentTileCount << endl;
        cout << "After redraw - mahjongTiles.maxTiles: " << mahjongTiles.maxTiles << endl;
        cout << "After redraw - mahjongTiles.tiles.size(): " << mahjongTiles.tiles.size() << endl;
    }

    void ApplyGrowthEffects() {
        // Track tile consumption for SHIFT cooldown
        tilesConsumed++;
        shiftCooldownTiles++;
        tilesConsumedSinceUpgrade++;

        // WHITE LOONG Purification: Turn leftmost tile to 0 every X tiles
        if (selectedLoongType == WHITE_LOONG) {
            purificationCounter++;
            if (purificationCounter >= purificationThreshold) {
                purificationCounter = 0;

                // Turn leftmost tile to 0
                if (!mahjongTiles.tiles.empty()) {
                    mahjongTiles.tiles[0] = Tile(0, PLAIN_TILES, false);
                    mahjongTiles.SortTiles();
                    cout << "🤍 PURIFICATION: Leftmost tile turned to 0!" << endl;
                }
            }
        }

        // Check if SHIFT power is ready
        if (!shiftPowerReady && shiftCooldownTiles >= shiftCooldownMax) {
            shiftPowerReady = true;
            cout << "*** SHIFT POWER READY: " << shiftPowerName << " ***" << endl;
        }

        // NEW UPGRADE TILE SYSTEM: Spawn upgrade tiles at age thresholds
        if (!upgradeSpawned && snakeAge >= nextUpgradeThreshold) {
            SpawnUpgradeTile();
        }

        // CROSS-LOONG UPGRADE SYSTEM: Spawn at specific ages 33, 66, 100
        if (!upgradeSpawned && (snakeAge == 33 || snakeAge == 66 || snakeAge == 100)) {
            cout << "*** CROSS-LOONG UPGRADE TRIGGERED at age " << snakeAge << "! ***" << endl;
            SpawnUpgradeTile();
        }

        if (slowGrowthActive) {
            // Only grow every 2 fruits
            if (fruitCounter % 2 == 0) {
                snake.addSegment = true;
            }
        } else if (collectorActive) {
            // Grow 2 per fruit
            snake.addSegment = true;
            snake.AddSegments(1); // Add extra segment
        } else {
            // Normal growth
            snake.addSegment = true;
        }
    }

    void ApplyFruitEffects() {
        // Turtle Mode: Every 3 fruits add 1 length
        if (turtleModeActive && fruitCounter % 3 == 0) {
            snake.AddSegments(1);
        }

        // Lucky Numbers: Every 5 fruits add 1 length
        if (luckyNumbersActive && fruitCounter % 5 == 0) {
            snake.AddSegments(1);
        }

        // Gambler: Lose 2 length every 7 fruits
        if (gamblerActive && fruitCounter % 7 == 0) {
            if (snake.body.size() > 5) { // Keep minimum size
                snake.body.pop_back();
                if (snake.body.size() > 5) {
                    snake.body.pop_back();
                }
            }
        }
    }

    // NEW UPGRADE TILE SYSTEM FUNCTIONS
    void SpawnUpgradeTile() {
        // Check if this is a cross-LOONG upgrade (33, 66, 100 age)
        if (snakeAge == 33 || snakeAge == 66 || snakeAge == 100) {
            // Cross-LOONG upgrades: Always from other LOONG (no SHIFT powers)
            vector<LoongType> otherTypes;
            for (int i = 0; i < 9; i++) { // 9 LOONG types total
                if (i != (int)selectedLoongType) {
                    otherTypes.push_back((LoongType)i);
                }
            }
            if (!otherTypes.empty()) {
                upgradeTileType = otherTypes[GetRandomValue(0, otherTypes.size() - 1)];
            } else {
                upgradeTileType = selectedLoongType; // Fallback
            }
            cout << "*** CROSS-LOONG UPGRADE at age " << snakeAge << "! ***" << endl;
        } else {
            // Regular upgrades: Always current LOONG
            upgradeTileType = selectedLoongType;
        }

        // Spawn the upgrade tile at a random position
        upgradeTilePosition = {
            (float)GetRandomValue(2, cellCount - 3),
            (float)GetRandomValue(2, cellCount - 3)
        };

        // Make sure it doesn't spawn on snake or food
        while (ElementInDeque(upgradeTilePosition, snake.body) ||
               (upgradeTilePosition.x == food.position.x && upgradeTilePosition.y == food.position.y)) {
            upgradeTilePosition = {
                (float)GetRandomValue(2, cellCount - 3),
                (float)GetRandomValue(2, cellCount - 3)
            };
        }

        upgradeSpawned = true;
        cout << "*** UPGRADE TILE SPAWNED: " << GetLoongTypeName(upgradeTileType) << " at (" << upgradeTilePosition.x << ", " << upgradeTilePosition.y << ") ***" << endl;
    }

    string GetLoongTypeName(LoongType type) {
        switch (type) {
            case BASIC_LOONG: return "Bamboo";
            case FIRE_LOONG: return "Fire";
            case WATER_LOONG: return "Water";
            case WHITE_LOONG: return "White";
            case EARTH_LOONG: return "Earth";
            case WIND_LOONG: return "Wind";
            case SHADOW_LOONG: return "Shadow";
            case CELESTIAL_LOONG: return "Celestial";
            case PATIENCE_LOONG: return "Patience";
            default: return "Unknown";
        }
    }

    string GetPowerName(LoongType t) {
        switch(t) {
            case BASIC_LOONG: return "Tile Wisdom";
            case FIRE_LOONG: return "Burning Tiles";
            case WATER_LOONG: return "Tidal Wave";
            case WHITE_LOONG: return "Zero Mastery";
            case EARTH_LOONG: return "Granite Will";
            case WIND_LOONG: return "Tornado Tiles";
            case SHADOW_LOONG: return "Nightmare Tiles";
            case CELESTIAL_LOONG: return "Cosmic Wisdom";
            case PATIENCE_LOONG: return "Pause Control";
            default: return "Unknown";
        }
    }

    void UpdateSnakeLength() {
        snakeLength = snake.body.size();
    }

    void CollectUpgradeTile() {
        upgradeSpawned = false;
        // Don't reset tilesConsumedSinceUpgrade - it's for SHIFT cooldown only

        // FIXED UPGRADE SYSTEM: Every 10 levels - 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, then every 10
        nextUpgradeThreshold += 10; // Simple: every 10 levels (10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120...)

        // Generate upgrade choices for the collected LOONG type
        GenerateUpgradeChoicesForLoong(upgradeTileType);
        cout << "*** COLLECTED " << GetLoongTypeName(upgradeTileType) << " UPGRADE TILE! Next threshold: " << nextUpgradeThreshold << " ***" << endl;
    }

    void CollectLatentUpgradeTile() {
        latentUpgradeSpawned = false;

        // Generate upgrade choices for the collected latent LOONG type
        GenerateUpgradeChoicesForLoong(latentUpgradeTileType);
        cout << "*** COLLECTED LATENT CULTIVATION " << GetLoongTypeName(latentUpgradeTileType) << " UPGRADE TILE! ***" << endl;
    }

    void DrawLatentUpgradeTile() {
        // Get the color for this LOONG type
        Color upgradeColor = WHITE;
        string upgradeSymbol = "?";

        switch (latentUpgradeTileType) {
            case BASIC_LOONG:
                upgradeColor = {34, 139, 34, 255}; // Forest Green
                upgradeSymbol = "B";
                break;
            case FIRE_LOONG:
                upgradeColor = {255, 69, 0, 255}; // Red Orange
                upgradeSymbol = "F";
                break;
            case WATER_LOONG:
                upgradeColor = {30, 144, 255, 255}; // Dodger Blue
                upgradeSymbol = "W";
                break;
            case WHITE_LOONG:
                upgradeColor = {248, 248, 255, 255}; // Ghost White
                upgradeSymbol = "P"; // P for Pure
                break;
            case EARTH_LOONG:
                upgradeColor = {139, 69, 19, 255}; // Saddle Brown
                upgradeSymbol = "E";
                break;
            case WIND_LOONG:
                upgradeColor = {192, 192, 192, 255}; // Silver
                upgradeSymbol = "A"; // A for Air
                break;
            case SHADOW_LOONG:
                upgradeColor = {75, 0, 130, 255}; // Indigo
                upgradeSymbol = "S";
                break;
            case CELESTIAL_LOONG:
                upgradeColor = {255, 215, 0, 255}; // Gold
                upgradeSymbol = "C";
                break;
        }

        // Draw upgrade tile with diamond/dragon symbol
        int tileX = gameAreaOffset + latentUpgradeTilePosition.x * cellSize;
        int tileY = gameAreaOffset + latentUpgradeTilePosition.y * cellSize;

        // Draw pulsing background with different timing to distinguish from normal upgrades
        static float pulseTimer = 0.0f;
        pulseTimer += GetFrameTime();
        float pulse = (sin(pulseTimer * 12) + 1) * 0.5f; // Faster pulse for latent cultivation

        // Outer glow effect - brighter for latent cultivation
        Rectangle glowRect = {(float)(tileX - 8), (float)(tileY - 8), (float)(cellSize + 16), (float)(cellSize + 16)};
        Color glowColor = upgradeColor;
        glowColor.a = (unsigned char)(150 + pulse * 100);
        DrawRectangleRec(glowRect, glowColor);

        // Main tile with double border for latent cultivation
        Rectangle tileRect = {(float)tileX, (float)tileY, (float)cellSize, (float)cellSize};
        DrawRectangleRec(tileRect, upgradeColor);
        DrawRectangleLinesEx(tileRect, 6, WHITE); // Thicker border

        // Draw diamond/dragon symbol in center
        int symbolSize = 32;
        int symbolX = tileX + cellSize/2 - MeasureText(upgradeSymbol.c_str(), symbolSize)/2;
        int symbolY = tileY + cellSize/2 - symbolSize/2;
        DrawText(upgradeSymbol.c_str(), symbolX, symbolY, symbolSize, BLACK);

        // Draw "LATENT" label above tile
        string typeName = "LATENT " + GetLoongTypeName(latentUpgradeTileType);
        int nameWidth = MeasureText(typeName.c_str(), 14);
        DrawText(typeName.c_str(), tileX + cellSize/2 - nameWidth/2, tileY - 25, 14, upgradeColor);
    }

    void GenerateUpgradeChoicesForLoong(LoongType loongType) {
        upgradeChoices.clear();

        // Find the LOONG data for this type
        int loongIndex = -1;
        for (int i = 0; i < (int)availableLOONGs.size(); i++) {
            if (availableLOONGs[i].type == loongType) {
                loongIndex = i;
                break;
            }
        }

        if (loongIndex == -1) return;

        LoongData& targetLoong = availableLOONGs[loongIndex];

        // Check if this is a cross-LOONG upgrade (other LOONG) or own LOONG
        bool isCrossLoong = (loongType != selectedLoongType);

        if (isCrossLoong) {
            // CROSS-LOONG: 2 out of 4 choices (exclude SHIFT powers)
            vector<int> availableUpgrades;
            for (int i = 0; i < (int)targetLoong.upgrades.size(); i++) {
                // Skip SHIFT power upgrades for cross-LOONG
                bool isShiftPower = false;
                string upgradeName = targetLoong.upgrades[i].name;
                if (upgradeName.find("Wisdom") != string::npos ||
                    upgradeName.find("Tiles") != string::npos ||
                    upgradeName.find("SHIFT") != string::npos ||
                    upgradeName.find("Sight") != string::npos ||
                    upgradeName.find("Will") != string::npos ||
                    upgradeName.find("Cosmic") != string::npos ||
                    upgradeName.find("Purification") != string::npos ||
                    upgradeName.find("Control") != string::npos) {
                    isShiftPower = true;
                }

                if (!isShiftPower && targetLoong.upgrades[i].level < targetLoong.upgrades[i].maxLevel) {
                    availableUpgrades.push_back(i);
                }
            }

            // Pick 2 random non-SHIFT upgrades
            for (int i = 0; i < 2 && availableUpgrades.size() > 0; i++) {
                int randomIndex = GetRandomValue(0, availableUpgrades.size() - 1);
                upgradeChoices.push_back(availableUpgrades[randomIndex]);
                availableUpgrades.erase(availableUpgrades.begin() + randomIndex);
            }
        } else {
            // OWN LOONG: 3 out of 5 choices (include SHIFT powers)
            vector<int> availableUpgrades;
            for (int i = 0; i < (int)targetLoong.upgrades.size(); i++) {
                if (targetLoong.upgrades[i].level < targetLoong.upgrades[i].maxLevel) {
                    availableUpgrades.push_back(i);
                }
            }

            // Pick 3 random upgrades (including SHIFT powers)
            for (int i = 0; i < 3 && availableUpgrades.size() > 0; i++) {
                int randomIndex = GetRandomValue(0, availableUpgrades.size() - 1);
                upgradeChoices.push_back(availableUpgrades[randomIndex]);
                availableUpgrades.erase(availableUpgrades.begin() + randomIndex);
            }
        }

        // Set the selected LOONG index temporarily for the upgrade screen
        int originalIndex = selectedLoongIndex;
        selectedLoongIndex = loongIndex;

        loongUpgradeSelection = 0;
        showLoongUpgrade = true;

        // Restore original index after showing upgrade
        // (This will be handled in the upgrade application)
    }

    ~Game()
    {
        SaveHighScore();
        UnloadSound(eatSound);
        UnloadSound(wallSound);

        // Unload LOONG image if loaded
        if (loongImageLoaded) {
            UnloadTexture(loongImage);
        }
        if (musicLoaded) {
            if (titleScreenMusic.stream.buffer != NULL) UnloadMusicStream(titleScreenMusic);
            if (loongSelectMusic.stream.buffer != NULL) UnloadMusicStream(loongSelectMusic);
            if (loongThemeMusic.stream.buffer != NULL) UnloadMusicStream(loongThemeMusic);
            if (finalStretchMusic1.stream.buffer != NULL) UnloadMusicStream(finalStretchMusic1);
            if (finalStretchMusic2.stream.buffer != NULL) UnloadMusicStream(finalStretchMusic2);
            if (gameOverMusic1.stream.buffer != NULL) UnloadMusicStream(gameOverMusic1);
            if (gameOverMusic2.stream.buffer != NULL) UnloadMusicStream(gameOverMusic2);
            if (backgroundMusic.stream.buffer != NULL) UnloadMusicStream(backgroundMusic);
            if (alternateMusic.stream.buffer != NULL) UnloadMusicStream(alternateMusic);
        }
        CloseAudioDevice();
    }



    void UpdateMusicAndCountdown()
    {
        float currentTime = GetTime();

        // Optimize music updates - only update every 16ms instead of every frame
        bool shouldUpdateMusic = (currentTime - lastMusicUpdateTime) >= musicUpdateInterval;
        if (shouldUpdateMusic) {
            lastMusicUpdateTime = currentTime;
        }

        // Handle title screen music
        if (musicLoaded && gameState == TITLE_SCREEN && shouldUpdateMusic) {
            if (!IsMusicStreamPlaying(titleScreenMusic)) {
                // Stop any other music
                if (IsMusicStreamPlaying(loongSelectMusic)) StopMusicStream(loongSelectMusic);
                if (IsMusicStreamPlaying(backgroundMusic)) StopMusicStream(backgroundMusic);
                if (IsMusicStreamPlaying(alternateMusic)) StopMusicStream(alternateMusic);

                PlayMusicStream(titleScreenMusic);
                cout << "Started playing title_screen.mp3" << endl;
            }
            UpdateMusicStream(titleScreenMusic);
            // Loop title music
            if (!IsMusicStreamPlaying(titleScreenMusic)) {
                SeekMusicStream(titleScreenMusic, 0.0f);
                PlayMusicStream(titleScreenMusic);
            }
        }
        // Handle LOONG selection music
        else if (musicLoaded && gameState == LOONG_SELECTION && shouldUpdateMusic) {
            if (!IsMusicStreamPlaying(loongSelectMusic)) {
                // Stop any other music
                if (IsMusicStreamPlaying(titleScreenMusic)) StopMusicStream(titleScreenMusic);
                if (IsMusicStreamPlaying(backgroundMusic)) StopMusicStream(backgroundMusic);
                if (IsMusicStreamPlaying(alternateMusic)) StopMusicStream(alternateMusic);

                PlayMusicStream(loongSelectMusic);
                cout << "Started playing select_loong.mp3" << endl;
            }
            UpdateMusicStream(loongSelectMusic);
            // Loop selection music
            if (!IsMusicStreamPlaying(loongSelectMusic)) {
                SeekMusicStream(loongSelectMusic, 0.0f);
                PlayMusicStream(loongSelectMusic);
            }
        }
        // Update background music and loop it - only during countdown and gameplay (not during choice window)
        else if (musicLoaded && (gameState == COUNTDOWN || gameState == PLAYING) && !showChoiceWindow && shouldUpdateMusic) {
            // Check if ornate level changed to final level (LEVEL_4_DRAGON) - only switch music once at level 4
            if (ornateLevel >= LEVEL_4_DRAGON && lastOrnateLevel < LEVEL_4_DRAGON) {
                cout << "Reached final ornate level 4! Switching to final_stretch music" << endl;

                // Stop all music safely
                if (IsMusicStreamPlaying(backgroundMusic)) {
                    StopMusicStream(backgroundMusic);
                }
                if (IsMusicStreamPlaying(alternateMusic)) {
                    StopMusicStream(alternateMusic);
                }
                if (IsMusicStreamPlaying(titleScreenMusic)) {
                    StopMusicStream(titleScreenMusic);
                }
                if (IsMusicStreamPlaying(loongSelectMusic)) {
                    StopMusicStream(loongSelectMusic);
                }

                // Play final stretch music (randomly selected)
                PlayMusicStream(alternateMusic);
                cout << "Started playing final_stretch music" << endl;

                lastOrnateLevel = ornateLevel;
            }
            // Start LOONG theme music if no music is playing and we're not at final level
            else if (ornateLevel < LEVEL_4_DRAGON && !IsMusicStreamPlaying(backgroundMusic) && !IsMusicStreamPlaying(alternateMusic)) {
                // Stop menu music if playing
                if (IsMusicStreamPlaying(titleScreenMusic)) {
                    StopMusicStream(titleScreenMusic);
                }
                if (IsMusicStreamPlaying(loongSelectMusic)) {
                    StopMusicStream(loongSelectMusic);
                }

                PlayMusicStream(backgroundMusic);
                cout << "Started playing LOONG theme music" << endl;
                lastOrnateLevel = ornateLevel;
            }

            // Update the currently playing music and ensure looping
            if (IsMusicStreamPlaying(backgroundMusic)) {
                UpdateMusicStream(backgroundMusic);
                // Check if music ended and restart for looping
                if (!IsMusicStreamPlaying(backgroundMusic)) {
                    SeekMusicStream(backgroundMusic, 0.0f);
                    PlayMusicStream(backgroundMusic); // Loop
                    cout << "Looped tian_tian.mp3" << endl;
                }
            }
            if (IsMusicStreamPlaying(alternateMusic)) {
                UpdateMusicStream(alternateMusic);
                // Check if music ended and restart for looping
                if (!IsMusicStreamPlaying(alternateMusic)) {
                    SeekMusicStream(alternateMusic, 0.0f);
                    PlayMusicStream(alternateMusic); // Loop
                    cout << "Looped final_stretch.mp3" << endl;
                }
            }
        }
        // Handle game over music
        else if (musicLoaded && gameState == GAME_OVER && shouldUpdateMusic) {
            if (currentGameOverMusic.stream.buffer != NULL) {
                UpdateMusicStream(currentGameOverMusic);
                // Loop game over music
                if (!IsMusicStreamPlaying(currentGameOverMusic)) {
                    SeekMusicStream(currentGameOverMusic, 0.0f);
                    PlayMusicStream(currentGameOverMusic);
                }
            }
        }

        if (gameState == COUNTDOWN) {
            // Handle countdown - comfortable speed, updated every frame
            countdownTimer -= GetFrameTime();
            if (countdownTimer <= 2.0f && countdownNumber == 3) {
                countdownNumber = 2;
                // Play "Two" sound - higher pitch for clarity
                SetSoundPitch(eatSound, 1.2f);
                PlaySound(eatSound);
            } else if (countdownTimer <= 1.0f && countdownNumber == 2) {
                countdownNumber = 1;
                // Play "One" sound - even higher pitch
                SetSoundPitch(eatSound, 1.5f);
                PlaySound(eatSound);
            } else if (countdownTimer <= 0.0f) {
                // Play "Go!" sound - highest pitch
                SetSoundPitch(eatSound, 2.0f);
                PlaySound(eatSound);
                // Reset sound pitch for normal gameplay
                SetSoundPitch(eatSound, 1.0f);
                gameState = PLAYING;

                // Trigger latent cultivation spawning now that the game has started
                if (!latentCultivationSpawned && selectedLatentLevel > 0) {
                    SpawnLatentCultivationUpgrades();
                    latentCultivationSpawned = true;
                }
            }
        }

        // Update number popup every frame for smooth animation
        numberPopup.Update(GetFrameTime());

        // Update mahjong win display every frame
        if (showMahjongWin) {
            mahjongWinTimer -= GetFrameTime();
            if (mahjongWinTimer <= 0) {
                showMahjongWin = false;
            }
        }

        // Update kong win display every frame
        if (showKongWin) {
            kongWinTimer -= GetFrameTime();
            if (kongWinTimer <= 0) {
                showKongWin = false;
            }
        }

        // Update phoenix rebirth effect
        if (showPhoenixRebirth) {
            phoenixRebirthTimer -= GetFrameTime();
            if (phoenixRebirthTimer <= 0) {
                showPhoenixRebirth = false;
            }
        }

        // Update cosmic wisdom effect
        if (showCosmicWisdom) {
            cosmicWisdomTimer -= GetFrameTime();
            if (cosmicWisdomTimer <= 0) {
                showCosmicWisdom = false;
            }
        }

        // SHIFT power cooldown is now tile-based, updated when tiles are consumed

        // Update choice window (no timer - let player take their time)
        if (showChoiceWindow) {
            // No auto-selection - player must choose
        }
    }

    void UpdateGameplay()
    {
        // Update latent cultivation upgrade spawning (SEPARATE from normal upgrades)
        if (!pendingLatentUpgrades.empty() && !latentUpgradeSpawned) {
            latentUpgradeSpawnTimer -= GetFrameTime();
            if (latentUpgradeSpawnTimer <= 0.0f) {
                // Spawn the next latent cultivation tile in queue
                LoongType nextUpgrade = pendingLatentUpgrades[0];
                pendingLatentUpgrades.erase(pendingLatentUpgrades.begin());

                // Spawn latent cultivation tile separately from normal upgrades
                latentUpgradeTileType = nextUpgrade;
                latentUpgradeTilePosition = {
                    (float)GetRandomValue(2, cellCount - 3),
                    (float)GetRandomValue(2, cellCount - 3)
                };

                // Make sure it doesn't spawn on snake, food, or normal upgrade tile
                while (ElementInDeque(latentUpgradeTilePosition, snake.body) ||
                       (latentUpgradeTilePosition.x == food.position.x && latentUpgradeTilePosition.y == food.position.y) ||
                       (upgradeSpawned && latentUpgradeTilePosition.x == upgradeTilePosition.x && latentUpgradeTilePosition.y == upgradeTilePosition.y)) {
                    latentUpgradeTilePosition = {
                        (float)GetRandomValue(2, cellCount - 3),
                        (float)GetRandomValue(2, cellCount - 3)
                    };
                }

                latentUpgradeSpawned = true;
                cout << "Spawned latent cultivation tile: " << GetLoongTypeName(nextUpgrade) << " at (" << latentUpgradeTilePosition.x << ", " << latentUpgradeTilePosition.y << ")" << endl;

                // Reset timer for next spawn
                latentUpgradeSpawnTimer = latentUpgradeSpawnDelay;
            }
        }

        if (gameState == PLAYING && !showChoiceWindow && !showLoongUpgrade && !isInExtraLifeMode && !isPaused) // Pause game during choice window, LOONG upgrade, extra life, or ability pause
        {
            snake.Update();
            CheckCollisionWithFood();
            if (isInExtraLifeMode || gameState != PLAYING) return; // Stop immediately if life used
            CheckCollisionWithEdges();
            if (isInExtraLifeMode || gameState != PLAYING) return; // Stop immediately if life used
            CheckCollisionWithTail();
        }
    }

    void Update()
    {
        // Legacy function - calls both updates
        UpdateMusicAndCountdown();
        UpdateGameplay();
    }

    void HandleInput()
    {
        int gamepad = 0;
        bool gpAvailable = IsGamepadAvailable(gamepad);

        // Input abstraction for menu navigation
        bool menuUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP));
        bool menuDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN));
        bool menuLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
        bool menuRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
        bool menuConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        bool menuBack = IsKeyPressed(KEY_BACKSPACE) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));

        // Handle gamepad sticks
        if (gpAvailable) {
            float threshold = 0.5f;
            stickUp = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y) < -threshold;
            stickDown = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y) > threshold;
            stickLeft = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X) < -threshold;
            stickRight = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X) > threshold;

            if (stickUp && !stickUpLast) menuUp = true;
            if (stickDown && !stickDownLast) menuDown = true;
            if (stickLeft && !stickLeftLast) menuLeft = true;
            if (stickRight && !stickRightLast) menuRight = true;
        }

        if (gameState == TITLE_SCREEN) {
            // Any click goes to LOONG selection
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                gameState = LOONG_SELECTION;
                cout << ">>> Entering LOONG Selection Screen! <<<" << endl;
            }
        }
        else if (gameState == LOONG_SELECTION) {
            // Handle mouse hover for LOONG selection
            Vector2 mousePos = GetMousePosition();
            int cardWidth = 350;
            int cardHeight = 200;
            int startX = (canvasWidth - (3 * cardWidth + 2 * 50)) / 2;
            int startY = 180;

            // Check mouse hover over LOONG cards (only unlocked ones)
            for (int i = 0; i < (int)availableLOONGs.size(); i++) {
                pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)i, FOUNDATION_BUILDING};
                bool isUnlocked = difficultyUnlocked[dragonKey];

                if (isUnlocked) {
                    int row = i / 3;
                    int col = i % 3;
                    int cardX = startX + col * (cardWidth + 50);
                    int cardY = startY + row * (cardHeight + 30);

                    Rectangle cardRect = {(float)cardX, (float)cardY, (float)cardWidth, (float)cardHeight};
                    if (CheckCollisionPointRec(mousePos, cardRect)) {
                        selectedLoongIndex = i;
                    }
                }
            }

            // Handle keyboard navigation (only unlocked dragons)
            if (menuLeft) {
                int newIndex = selectedLoongIndex;
                do {
                    newIndex = (newIndex - 1 + availableLOONGs.size()) % availableLOONGs.size();
                    pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)newIndex, FOUNDATION_BUILDING};
                    if (difficultyUnlocked[dragonKey]) {
                        selectedLoongIndex = newIndex;
                        cout << "Selected " << availableLOONGs[selectedLoongIndex].name << endl;
                        break;
                    }
                } while (newIndex != selectedLoongIndex);
            }
            if (menuRight) {
                int newIndex = selectedLoongIndex;
                do {
                    newIndex = (newIndex + 1) % availableLOONGs.size();
                    pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)newIndex, FOUNDATION_BUILDING};
                    if (difficultyUnlocked[dragonKey]) {
                        selectedLoongIndex = newIndex;
                        cout << "Selected " << availableLOONGs[selectedLoongIndex].name << endl;
                        break;
                    }
                } while (newIndex != selectedLoongIndex);
            }
            if (menuUp) {
                int newIndex = selectedLoongIndex;
                do {
                    newIndex = (newIndex - 3 + availableLOONGs.size()) % availableLOONGs.size();
                    pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)newIndex, FOUNDATION_BUILDING};
                    if (difficultyUnlocked[dragonKey]) {
                        selectedLoongIndex = newIndex;
                        cout << "Selected " << availableLOONGs[selectedLoongIndex].name << endl;
                        break;
                    }
                } while (newIndex != selectedLoongIndex);
            }
            if (menuDown) {
                int newIndex = selectedLoongIndex;
                do {
                    newIndex = (newIndex + 3) % availableLOONGs.size();
                    pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)newIndex, FOUNDATION_BUILDING};
                    if (difficultyUnlocked[dragonKey]) {
                        selectedLoongIndex = newIndex;
                        cout << "Selected " << availableLOONGs[selectedLoongIndex].name << endl;
                        break;
                    }
                } while (newIndex != selectedLoongIndex);
            }

            // Confirm selection with Enter or mouse click (only if unlocked)
            if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                pair<LoongType, DifficultyLevel> dragonKey = {(LoongType)selectedLoongIndex, FOUNDATION_BUILDING};
                bool isUnlocked = difficultyUnlocked[dragonKey];

                if (isUnlocked) {
                    selectedLoongType = availableLOONGs[selectedLoongIndex].type;

                    // Initialize Shift power for the new LOONG
                    InitializeShiftPower();

                    // Load LOONG-specific theme music
                    LoadLoongThemeMusic();

                    // Load LOONG-specific image
                    LoadLoongImage();

                    // Update background color to DARK version of selected LOONG for high contrast
                    LoongData& selectedLoong = availableLOONGs[selectedLoongIndex];
                    currentBackgroundColor = selectedLoong.primaryColor;
                    // Make it MUCH darker for high contrast with bright LOONG snake
                    currentBackgroundColor.r = currentBackgroundColor.r / 4; // 25% of original
                    currentBackgroundColor.g = currentBackgroundColor.g / 4;
                    currentBackgroundColor.b = currentBackgroundColor.b / 4;
                    currentBackgroundColor.a = 255; // Solid dark background

                    gameState = DIFFICULTY_SELECTION; // Go to difficulty selection first
                    cout << ">>> Selected " << availableLOONGs[selectedLoongIndex].name << " - background and music updated! <<<" << endl;
                } else {
                    cout << "Dragon " << availableLOONGs[selectedLoongIndex].name << " is locked! Beat the previous dragon first." << endl;
                }
            }
        }
        else if (gameState == DIFFICULTY_SELECTION) {
            // Handle mouse hover for difficulty selection
            Vector2 mousePos = GetMousePosition();
            int startY = 220;
            int spacing = 80;

            for (int i = 0; i < 5; i++) {
                pair<LoongType, DifficultyLevel> diffKey = {selectedLoongType, (DifficultyLevel)i};
                bool isUnlocked = (i == 0) || difficultyUnlocked[diffKey];

                if (isUnlocked) {
                    int diffY = startY + i * spacing;
                    Rectangle diffRect = {150.0f, (float)(diffY - 10), 700.0f, 70.0f};

                    if (CheckCollisionPointRec(mousePos, diffRect)) {
                        selectedDifficulty = (DifficultyLevel)i;
                    }
                }
            }

            // Handle keyboard navigation
            if (menuUp) {
                // Find previous unlocked difficulty
                for (int i = selectedDifficulty - 1; i >= 0; i--) {
                    pair<LoongType, DifficultyLevel> diffKey = {selectedLoongType, (DifficultyLevel)i};
                    if (i == 0 || difficultyUnlocked[diffKey]) { // Foundation Building always unlocked
                        selectedDifficulty = (DifficultyLevel)i;
                        break;
                    }
                }
            }
            if (menuDown) {
                // Find next unlocked difficulty
                for (int i = selectedDifficulty + 1; i < 5; i++) {
                    pair<LoongType, DifficultyLevel> diffKey = {selectedLoongType, (DifficultyLevel)i};
                    if (difficultyUnlocked[diffKey]) {
                        selectedDifficulty = (DifficultyLevel)i;
                        break;
                    }
                }
            }

            // Back button functionality (use BACKSPACE instead of ESC)
            if (menuBack) {
                gameState = LOONG_SELECTION;
            }

            // Confirm difficulty selection
            if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Check if clicking on a valid difficulty option
                bool validClick = false;
                for (int i = 0; i < 5; i++) {
                    pair<LoongType, DifficultyLevel> diffKey = {selectedLoongType, (DifficultyLevel)i};
                    bool isUnlocked = (i == 0) || difficultyUnlocked[diffKey];

                    if (isUnlocked && selectedDifficulty == i) {
                        int diffY = startY + i * spacing;
                        Rectangle diffRect = {150.0f, (float)(diffY - 10), 700.0f, 70.0f};

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if (CheckCollisionPointRec(mousePos, diffRect)) {
                                validClick = true;
                            }
                        } else if (menuConfirm) {
                            validClick = true;
                        }
                        break;
                    }
                }

                if (validClick || menuConfirm) {
                    ApplyDifficultySettings();
                    gameState = INSTRUCTION_SCREEN;
                }
            }
        }
        else if (gameState == INSTRUCTION_SCREEN) {
            // Wait for player to click to start countdown
            if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                StartCountdown();
                // Start LOONG theme music when countdown begins
                if (musicLoaded && backgroundMusic.stream.buffer != NULL) {
                    // Stop menu music
                    if (IsMusicStreamPlaying(loongSelectMusic)) {
                        StopMusicStream(loongSelectMusic);
                    }
                    PlayMusicStream(backgroundMusic);
                    cout << "Started LOONG theme music for gameplay" << endl;
                }
            }
        }
        else if (gameState == COUNTDOWN) {
            // No input during countdown
        }
        else if (gameState == PLAYING) {
            // Handle extra life mode first
            if (isInExtraLifeMode) {
                // Let player move mouse to reposition, then click to continue
                Vector2 mousePos = GetMousePosition();
                snake.UpdateDirectionFromMouse(mousePos);
                if (gpAvailable) {
                    snake.UpdateDirectionFromGamepad(gamepad);
                }

                if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    isInExtraLifeMode = false;
                    cout << "Extra life resumed!" << endl;
                }
            }
            // Handle LOONG upgrade input
            else if (showLoongUpgrade) {
                // Arrow keys for upgrade selection (dynamic based on available choices)
                int numChoices = upgradeChoices.size();
                if (menuUp) {
                    loongUpgradeSelection = (loongUpgradeSelection - 1 + numChoices) % numChoices;
                }
                if (menuDown) {
                    loongUpgradeSelection = (loongUpgradeSelection + 1) % numChoices;
                }

                // Mouse wheel support for upgrade selection
                float wheelMove = GetMouseWheelMove();
                if (wheelMove > 0) {
                    loongUpgradeSelection = (loongUpgradeSelection - 1 + numChoices) % numChoices;
                } else if (wheelMove < 0) {
                    loongUpgradeSelection = (loongUpgradeSelection + 1) % numChoices;
                }

                // Mouse hover detection for upgrade selection
                Vector2 mousePos = GetMousePosition();
                int windowWidth = 800;
                int windowHeight = 600;
                int windowX = canvasWidth/2 - windowWidth/2;
                int windowY = canvasHeight/2 - windowHeight/2;

                for (int i = 0; i < (int)upgradeChoices.size(); i++) {
                    int choiceY = windowY + 130 + i * 120;
                    Rectangle choiceRect = {(float)(windowX + 20), (float)(choiceY - 10), (float)(windowWidth - 40), 100.0f};
                    if (CheckCollisionPointRec(mousePos, choiceRect)) {
                        loongUpgradeSelection = i;
                    }
                }

                // Confirm upgrade selection
                if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ApplyLoongUpgrade(loongUpgradeSelection);
                    cout << "🐉 LOONG UPGRADE APPLIED! 🐉" << endl;
                }
            }
            // Handle choice window input
            else if (showChoiceWindow) {
                // Mouse hover detection for choice selection
                Vector2 mousePos = GetMousePosition();
                int windowWidth = 600;
                int windowHeight = 400;
                int windowX = canvasWidth/2 - windowWidth/2;
                int windowY = canvasHeight/2 - windowHeight/2;

                // Check which choice is being hovered
                for (int i = 0; i < 3; i++) {
                    int choiceY = windowY + 120 + i * 80;
                    Rectangle choiceRect = {(float)(windowX + 20), (float)(choiceY - 5), (float)(windowWidth - 40), 70.0f};

                    if (CheckCollisionPointRec(mousePos, choiceRect)) {
                        selectedChoice = i;
                        break;
                    }
                }

                // Mouse wheel support for choice selection
                float wheelMove = GetMouseWheelMove();
                if (wheelMove > 0) {
                    if (selectedChoice > 0) {
                        selectedChoice--;
                    }
                } else if (wheelMove < 0) {
                    if (selectedChoice < 2) {
                        selectedChoice++;
                    }
                }

                // Left click to confirm choice
                if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ApplyChoice(selectedChoice);
                    showChoiceWindow = false;
                }

                // Enter/Space to confirm choice
                if (menuConfirm) {
                    ApplyChoice(selectedChoice);
                    showChoiceWindow = false;
                }
            }
            else {
                // Handle mouse clicks for arrow movement with hold detection
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    leftClickHoldTime = 0.0f;
                    mahjongTiles.MoveArrowLeft(); // Immediate single move
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    rightClickHoldTime = 0.0f;
                    mahjongTiles.MoveArrowRight(); // Immediate single move
                }

                // Track hold time and trigger far movement after threshold
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    leftClickHoldTime += GetFrameTime();
                    if (leftClickHoldTime >= HOLD_THRESHOLD) {
                        mahjongTiles.MoveArrowToFarLeft();
                        leftClickHoldTime = 0.0f; // Reset to prevent repeated calls
                    }
                } else {
                    leftClickHoldTime = 0.0f;
                }

                if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                    rightClickHoldTime += GetFrameTime();
                    if (rightClickHoldTime >= HOLD_THRESHOLD) {
                        mahjongTiles.MoveArrowToFarRight();
                        rightClickHoldTime = 0.0f; // Reset to prevent repeated calls
                    }
                } else {
                    rightClickHoldTime = 0.0f;
                }

                // --- Tile Selection Hold Logic ---
                bool selectLeft = IsKeyDown(KEY_Q) || (gpAvailable && (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_2)));
                bool selectRight = IsKeyDown(KEY_E) || (gpAvailable && (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)));

                if (selectLeft || selectRight) {
                    selectRepeatTimer += GetFrameTime();
                    if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E) || (gpAvailable && (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)))) {
                        if (selectLeft) mahjongTiles.MoveArrowLeft();
                        if (selectRight) mahjongTiles.MoveArrowRight();
                        selectRepeatTimer = 0.0f;
                    } else if (selectRepeatTimer >= REPEAT_DELAY) {
                        if (selectLeft) mahjongTiles.MoveArrowLeft();
                        if (selectRight) mahjongTiles.MoveArrowRight();
                        selectRepeatTimer = REPEAT_DELAY - REPEAT_RATE;
                    }
                } else {
                    selectRepeatTimer = 0.0f;
                }

                // --- Snake Movement Controls ---
                if (menuUp && snake.direction.y != 1) snake.direction = {0, -1};
                if (menuDown && snake.direction.y != -1) snake.direction = {0, 1};
                if (menuLeft && snake.direction.x != 1) snake.direction = {-1, 0};
                if (menuRight && snake.direction.x != -1) snake.direction = {1, 0};

                // Controller tile selection
                if (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) mahjongTiles.MoveArrowLeft();
                if (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) mahjongTiles.MoveArrowRight();

                // Mouse wheel support for tile selection (same as A/D)
                float wheelMove = GetMouseWheelMove();
                if (wheelMove > 0) {
                    mahjongTiles.MoveArrowLeft(); // Wheel up = move left
                } else if (wheelMove < 0) {
                    mahjongTiles.MoveArrowRight(); // Wheel down = move right
                }

                // Space bar for reshuffle
                if (IsKeyPressed(KEY_SPACE) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
                    mahjongTiles.ReshuffleTiles();
                }

                // SHIFT key for dragon special power
                if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) {
                    if (shiftPowerReady) {
                        ActivateShiftPower();
                    } else {
                        int tilesNeeded = shiftCooldownMax - shiftCooldownTiles;
                        cout << "*** SHIFT POWER ON COOLDOWN: " << tilesNeeded << " tiles remaining ***" << endl;
                    }
                }
            }
        }
        else if (gameState == GAME_OVER) {
            // Return to LOONG selection for new game
            if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Reset game state completely
                ResetGame();
                gameState = LOONG_SELECTION;
                cout << "🐉 Returning to LOONG Selection after Game Over! 🐉" << endl;
            }
        }
        else if (gameState == CULTIVATION_SUCCESS) {
            // Return to LOONG selection after cultivation success
            if (menuConfirm || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Reset game state completely like game over
                ResetGame();
                gameState = LOONG_SELECTION;
                cout << "🐉 Returning to LOONG Selection after Cultivation Success! 🐉" << endl;
            }
        }

        // Global audio controls (work in any state)
        if (IsKeyPressed(KEY_M) || (gpAvailable && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT))) {
            ToggleMute();
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) { // + key
            SetVolume(masterVolume + 0.1f);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) { // - key
            SetVolume(masterVolume - 0.1f);
        }

        // Update last stick states for next frame
        stickUpLast = stickUp;
        stickDownLast = stickDown;
        stickLeftLast = stickLeft;
        stickRightLast = stickRight;
    }

    void StartCountdown() {
        gameState = COUNTDOWN;
        countdownTimer = 3.0f; // 3 seconds countdown (comfortable speed)
        countdownNumber = 3;
    }

    void Draw()
    {
        if (gameState == TITLE_SCREEN) {
            DrawTitleScreen();
        } else if (gameState == LOONG_SELECTION) {
            DrawLoongSelectionScreen();
        } else if (gameState == DIFFICULTY_SELECTION) {
            DrawDifficultySelectionScreen();
        } else if (gameState == INSTRUCTION_SCREEN) {
            DrawInstructionScreen();
        } else if (gameState == COUNTDOWN) {
            DrawCountdownScreen();
        } else if (gameState == GAME_OVER) {
            DrawGameOverScreen();
        } else if (gameState == CULTIVATION_SUCCESS) {
            DrawCultivationSuccessScreen();
        } else {
            DrawGameScreen();

            // Draw LOONG upgrade overlay if active
            if (showLoongUpgrade) {
                DrawLoongUpgradeScreen();
            }
            // Draw choice window overlay if active
            else if (showChoiceWindow) {
                DrawChoiceWindow();
            }

            // Draw debug UI
            if (showDebugUI) {
                DrawDebugUI();
            }

            // Draw extra life overlay if active
            if (isInExtraLifeMode) {
                DrawExtraLifeOverlay();
            }

            // Draw special LOONG ability effects
            if (showPhoenixRebirth) {
                DrawPhoenixRebirthEffect();
            }

            if (showCosmicWisdom) {
                DrawCosmicWisdomEffect();
            }
        }
    }

    void DrawTitleScreen()
    {
        ClearBackground(BLACK);

        // Draw title - properly centered
        int titleWidth = MeasureText("LOONG MAHJONG", 80);
        DrawText("LOONG MAHJONG", canvasWidth/2 - titleWidth/2, 250, 80, GOLD);

        // Draw start button - properly centered
        int startWidth = MeasureText("START", 50);
        DrawText("START", canvasWidth/2 - startWidth/2, 400, 50, WHITE);

        // Draw instructions - properly centered
        int instWidth = MeasureText("Click anywhere to start", 30);
        DrawText("Click anywhere to start", canvasWidth/2 - instWidth/2, 500, 30, GRAY);

        // Show high score - properly centered
        char highScoreText[50];
        sprintf(highScoreText, "High Score: %d", highScore);
        int hsWidth = MeasureText(highScoreText, 24);
        DrawText(highScoreText, canvasWidth/2 - hsWidth/2, 600, 24, WHITE);

        // Game rules - properly centered
        DrawText("WASD to Move | Q/E to Select | SHIFT for Power", canvasWidth/2 - 250, 750, 24, YELLOW);
        DrawText("Match tiles to evolve your Dragon!", canvasWidth/2 - 180, 800, 24, WHITE);
    }

    void DrawDifficultySelectionScreen()
    {
        ClearBackground(BLACK);

        // Draw title
        int titleWidth = MeasureText("SELECT CULTIVATION LEVEL", 60);
        DrawText("SELECT CULTIVATION LEVEL", canvasWidth/2 - titleWidth/2, 100, 60, GOLD);

        // Show selected LOONG
        LoongData& selectedLoong = availableLOONGs[selectedLoongIndex];
        char loongText[100];
        sprintf(loongText, "LOONG: %s", selectedLoong.name.c_str());
        int loongWidth = MeasureText(loongText, 30);
        DrawText(loongText, canvasWidth/2 - loongWidth/2, 160, 30, selectedLoong.primaryColor);

        // Draw difficulty options
        int startY = 220;
        int spacing = 80;

        for (int i = 0; i < 5; i++) {
            pair<LoongType, DifficultyLevel> diffKey = {selectedLoongType, (DifficultyLevel)i};
            bool isUnlocked = (i == 0) || difficultyUnlocked[diffKey]; // Foundation Building always unlocked
            bool isSelected = (selectedDifficulty == i);

            Color textColor = isUnlocked ? WHITE : GRAY;
            if (isSelected) textColor = GOLD;
            if (!isUnlocked) textColor = DARKGRAY;

            int diffY = startY + i * spacing;

            // Draw difficulty name
            DrawText(difficultyNames[i].c_str(), 200, diffY, 32, textColor);

            // Draw description
            DrawText(difficultyDescriptions[i].c_str(), 200, diffY + 35, 20, textColor);

            // Show high score for this difficulty
            if (isUnlocked) {
                int highScore = (loongHighScores.count(diffKey) > 0) ? loongHighScores[diffKey] : 0;
                char highScoreText[50];
                sprintf(highScoreText, "High Score: %d", highScore);
                DrawText(highScoreText, 1000, diffY + 10, 24, textColor);
            } else {
                DrawText("LOCKED", 1000, diffY + 10, 24, RED);
            }

            // Draw selection indicator
            if (isSelected && isUnlocked) {
                DrawText(">", 150, diffY + 5, 40, GOLD);
            }
        }

        // Instructions
        DrawText("UP/DOWN: Navigate  ENTER: Select  BACKSPACE: Back", canvasWidth/2 - 280, canvasHeight - 100, 24, WHITE);

        // Audio controls
        int audioY = canvasHeight - 150;
        DrawText("Audio Controls:", 50, audioY, 20, YELLOW);

        // Mute button
        Rectangle muteButton = {50, (float)(audioY + 30), 100, 30};
        Color muteColor = isMuted ? RED : GREEN;
        DrawRectangleRec(muteButton, muteColor);
        DrawRectangleLinesEx(muteButton, 2, WHITE);
        const char* muteText = isMuted ? "UNMUTE" : "MUTE";
        int muteTextWidth = MeasureText(muteText, 16);
        DrawText(muteText, 50 + (100 - muteTextWidth)/2, audioY + 38, 16, WHITE);

        // Volume slider
        DrawText("Volume:", 170, audioY + 35, 16, WHITE);
        Rectangle volumeSlider = {240, (float)(audioY + 35), 200, 20};
        DrawRectangleRec(volumeSlider, DARKGRAY);
        DrawRectangleLinesEx(volumeSlider, 2, WHITE);

        // Volume indicator
        float volumeWidth = volumeSlider.width * masterVolume;
        Rectangle volumeIndicator = {volumeSlider.x, volumeSlider.y, volumeWidth, volumeSlider.height};
        DrawRectangleRec(volumeIndicator, BLUE);

        // Volume handle
        float handleX = volumeSlider.x + volumeWidth - 5;
        Rectangle volumeHandle = {handleX, volumeSlider.y - 2, 10, 24};
        DrawRectangleRec(volumeHandle, WHITE);

        // Handle audio control interactions
        Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mousePos, muteButton)) {
                ToggleMute();
            } else if (CheckCollisionPointRec(mousePos, volumeSlider)) {
                float newVolume = (mousePos.x - volumeSlider.x) / volumeSlider.width;
                SetVolume(newVolume);
            }
        }

        // Latent upgrade display
        DrawLatentUpgradeInfo();
    }

    void ApplyDifficultySettings() {
        cout << "Applying difficulty: " << difficultyNames[selectedDifficulty] << endl;

        // Reset difficulty-specific variables
        mahjongWinsRequired = 0;
        mahjongWinsAchieved = 0;

        switch (selectedDifficulty) {
            case FOUNDATION_BUILDING:
                // 3 Extra Lives
                extraLives = 3;
                phoenixRebirthCharges = 0;
                mahjongTiles.SetTileCount(4); // Start with 4 tiles
                // Generate normal tiles
                mahjongTiles.GenerateRandomTiles(0);
                break;

            case GOLDEN_CORE:
                // 1 Life, no Extra Lives
                extraLives = 0;
                phoenixRebirthCharges = 0;
                mahjongTiles.SetTileCount(4); // Start with 4 tiles
                // Generate normal tiles
                mahjongTiles.GenerateRandomTiles(0);
                break;

            case NASCENT_SOUL:
                // Start with 7 tiles, need 2 Mahjongs to reach 13 tiles
                extraLives = 1;
                phoenixRebirthCharges = 0;
                mahjongTiles.SetTileCount(7); // Start with 7 tiles
                // Generate normal tiles
                mahjongTiles.GenerateRandomTiles(0);
                mahjongWinsRequired = 2; // Need 2 Mahjongs before can win
                break;

            case ASCENSION:
                // Start with 10 tiles, need 5 Mahjongs to win
                extraLives = 1;
                phoenixRebirthCharges = 0;
                mahjongTiles.SetTileCount(10); // Start with 10 tiles
                // Generate normal tiles
                mahjongTiles.GenerateRandomTiles(0);
                mahjongWinsRequired = 5; // Need 5 Mahjongs before can win
                break;

            case IMMORTAL_SAGE:
                // Start with 13 tiles, need 5 Mahjongs to win
                extraLives = 1;
                phoenixRebirthCharges = 0;
                mahjongTiles.SetTileCount(13); // Start with 13 tiles
                // Generate normal tiles
                mahjongTiles.GenerateRandomTiles(0);
                mahjongWinsRequired = 5; // Need 5 Mahjongs before can win
                ornateLevel = LEVEL_3_INTRICATE; // Start at ornate level 3.5 (need to reach 4 to win)
                break;
        }

        cout << "Difficulty applied: Lives=" << extraLives << ", Tiles=" << mahjongTiles.maxTiles
             << ", MahjongWinsRequired=" << mahjongWinsRequired << endl;
    }

    void InitializeDifficultySystem() {
        // Initialize all difficulties as locked except Foundation Building
        for (int loong = 0; loong < 9; loong++) {
            for (int diff = 0; diff < 5; diff++) {
                pair<LoongType, DifficultyLevel> key = {(LoongType)loong, (DifficultyLevel)diff};
                difficultyUnlocked[key] = (diff == 0 && loong == 0); // Only Fa Cai Foundation Building unlocked initially
                loongHighScores[key] = 0; // Initialize high scores to 0
            }
        }

        // Load saved progress
        LoadProgressData();

        // Set selectedLoongIndex to first unlocked dragon
        selectedLoongIndex = 0;
        for (int i = 0; i < 9; i++) {
            pair<LoongType, DifficultyLevel> key = {(LoongType)i, FOUNDATION_BUILDING};
            if (difficultyUnlocked[key]) {
                selectedLoongIndex = i;
                break;
            }
        }

        cout << "Difficulty system initialized with sequential dragon unlocking" << endl;
    }

    void UnlockNextDifficulty() {
        // Unlock the next difficulty level for this specific LOONG
        if (selectedDifficulty < IMMORTAL_SAGE) {
            DifficultyLevel nextDiff = (DifficultyLevel)(selectedDifficulty + 1);
            pair<LoongType, DifficultyLevel> nextKey = {selectedLoongType, nextDiff};

            if (!difficultyUnlocked[nextKey]) {
                difficultyUnlocked[nextKey] = true;
                cout << "🎉 UNLOCKED: " << difficultyNames[nextDiff] << " for "
                     << availableLOONGs[selectedLoongIndex].name << "! 🎉" << endl;
            }
        }

        // Check if we should unlock the next dragon (Foundation Building difficulty only)
        if (selectedDifficulty == FOUNDATION_BUILDING) {
            // Dragon unlock order: Fa Cai -> Hong Zhong -> Bai Ban -> Long Wang -> Huang Di -> Qing Long -> Ju Long -> Ren Long -> Shen Long
            LoongType nextDragon = (LoongType)((int)selectedLoongType + 1);
            if (nextDragon < 9) { // Valid next dragon exists
                pair<LoongType, DifficultyLevel> nextDragonKey = {nextDragon, FOUNDATION_BUILDING};

                if (!difficultyUnlocked[nextDragonKey]) {
                    difficultyUnlocked[nextDragonKey] = true;
                    cout << "🐉 UNLOCKED NEW DRAGON: " << availableLOONGs[(int)nextDragon].name << "! 🐉" << endl;
                }
            }
        }

        // Save progress after unlocking
        SaveProgressData();
    }

    void DrawCountdownScreen()
    {
        // Draw game background
        ClearBackground(currentBackgroundColor);

        // Draw ornate background pattern based on level
        if (ornateLevel > LEVEL_1_NONE) {
            DrawOrnateBackground(ornateLevel);
        }

        // Draw semi-transparent overlay
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {0, 0, 0, 150});

        // Draw countdown number - large and centered
        char countdownText[10];
        sprintf(countdownText, "%d", countdownNumber);
        int textWidth = MeasureText(countdownText, 200);
        DrawText(countdownText, canvasWidth/2 - textWidth/2, canvasHeight/2 - 100, 200, GOLD);

        // Draw "GET READY" text
        int readyWidth = MeasureText("GET READY!", 60);
        DrawText("GET READY!", canvasWidth/2 - readyWidth/2, canvasHeight/2 + 150, 60, WHITE);
    }

    void DrawGameOverScreen()
    {
        ClearBackground(BLACK);

        // Draw "GAME OVER" - large and centered
        int gameOverWidth = MeasureText("GAME OVER", 80);
        DrawText("GAME OVER", canvasWidth/2 - gameOverWidth/2, canvasHeight/2 - 100, 80, RED);

        // Draw final score
        char scoreText[50];
        sprintf(scoreText, "Final Score: %d", finalScore);
        int scoreWidth = MeasureText(scoreText, 40);
        DrawText(scoreText, canvasWidth/2 - scoreWidth/2, canvasHeight/2, 40, WHITE);

        // Draw high score if achieved
        if (finalScore == highScore && finalScore > 0) {
            int newHighWidth = MeasureText("NEW HIGH SCORE!", 30);
            DrawText("NEW HIGH SCORE!", canvasWidth/2 - newHighWidth/2, canvasHeight/2 + 50, 30, GOLD);
        }

        // Draw try again instruction
        int tryAgainWidth = MeasureText("Click to Try Again", 30);
        DrawText("Click to Try Again", canvasWidth/2 - tryAgainWidth/2, canvasHeight/2 + 150, 30, GRAY);
    }

    void DrawCultivationSuccessScreen()
    {
        // Draw beautiful gold background
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {20, 20, 20, 255});

        // Draw level 3 ornate background for beauty
        DrawOrnateBackground(3);

        // Draw main title
        int titleWidth = MeasureText("CULTIVATION SUCCESS", 80);
        DrawText("CULTIVATION SUCCESS", canvasWidth/2 - titleWidth/2, canvasHeight/2 - 150, 80, GOLD);

        // Draw LOONG achievement
        int loongWidth = MeasureText("LOONG ACHIEVED!", 60);
        DrawText("LOONG ACHIEVED!", canvasWidth/2 - loongWidth/2, canvasHeight/2 - 80, 60, WHITE);

        // Draw tripled score
        char scoreText[100];
        sprintf(scoreText, "TRIPLED FINAL SCORE: %d", score);
        int scoreWidth = MeasureText(scoreText, 50);
        DrawText(scoreText, canvasWidth/2 - scoreWidth/2, canvasHeight/2 - 10, 50, YELLOW);

        // Draw congratulations
        int congratsWidth = MeasureText("You have mastered the way of the Loong!", 30);
        DrawText("You have mastered the way of the Loong!", canvasWidth/2 - congratsWidth/2, canvasHeight/2 + 60, 30, LIGHTGRAY);

        // Draw restart instruction
        int instWidth = MeasureText("Left Click to Play Again", 24);
        DrawText("Left Click to Play Again", canvasWidth/2 - instWidth/2, canvasHeight/2 + 120, 24, GREEN);
    }

    void DrawGameScreen()
    {
        // Draw outer mouse control area background
        ClearBackground(currentBackgroundColor);

        // Draw ornate background pattern based on level
        if (ornateLevel > LEVEL_1_NONE) {
            DrawOrnateBackground(ornateLevel);
        }

        {
            // Draw normal game elements
            // Draw inner game area with LOONG-themed colors
            Rectangle gameArea = {(float)(gameAreaOffset - 5), (float)(gameAreaOffset - 5),
                                 (float)(cellSize * cellCount + 10), (float)(cellSize * cellCount + 10)};

            // Use DARK LOONG colors for game board - high contrast
            Color boardColor = currentBackgroundColor;
            boardColor.a = 200; // Semi-transparent dark background
            Color borderColor = currentBackgroundColor;
            // Make border slightly brighter but still dark
            borderColor.r = min(100, borderColor.r + 30); // Cap at 100 to stay dark
            borderColor.g = min(100, borderColor.g + 30);
            borderColor.b = min(100, borderColor.b + 30);
            borderColor.a = 255; // Solid border

            DrawRectangleRec(gameArea, boardColor);
            DrawRectangleLinesEx(gameArea, 5, borderColor);

            // Draw compact tile display above game area
            DrawCompactTileDisplay();

            // Draw game objects - show next tile with correct type and color
            food.Draw(mahjongTiles.nextTile);

            // Draw upgrade tile if spawned
            if (upgradeSpawned) {
                DrawUpgradeTile();
            }

            // Draw latent cultivation tile if spawned
            if (latentUpgradeSpawned) {
                DrawLatentUpgradeTile();
            }

            // Get dragon colors based on selected LOONG
            Color dragonBodyColor, dragonScaleColor;
            GetDragonColors(dragonBodyColor, dragonScaleColor);
            snake.Draw(dragonBodyColor, dragonScaleColor);

            // Draw mouse position indicator
            Vector2 mousePos = GetMousePosition();
            DrawCircleV(mousePos, 6, darkGreen);
            DrawCircleV(mousePos, 3, green);

            // Draw number popup
            numberPopup.Draw();

            // Draw mahjong win celebration
            if (showMahjongWin) {
                // Gold text with transparent background in center
                DrawText("MAHJONG!", canvasWidth/2 - 120, canvasHeight/2 - 30, 60, GOLD);
                DrawText("+5 POINTS!", canvasWidth/2 - 100, canvasHeight/2 + 20, 40, GOLD);
            }

            // Draw kong win celebration
            if (showKongWin) {
                // Jade color text with transparent background in center
                Color jadeColor = {0, 168, 107, 255}; // Jade green
                DrawText("KONG!", canvasWidth/2 - 80, canvasHeight/2 - 30, 60, jadeColor);
                DrawText("+5 POINTS!", canvasWidth/2 - 100, canvasHeight/2 + 20, 40, jadeColor);
            }

            // Draw UI elements
            DrawUI();
        }
    }

    void DrawCompactTileDisplay()
    {
        // Draw enhanced tile display above the game area
        int tileDisplayY = 40;
        int tileDisplayX = gameAreaOffset;

        // Black background for tile display with white border - wider for 13 tiles + KEEP
        int maxTileWidth = 13 * 80 + 100 + 40; 
        Rectangle tileBackground = {(float)(tileDisplayX - 15), (float)(tileDisplayY - 10),
                                   (float)maxTileWidth, 120.0f};
        DrawRectangleRec(tileBackground, BLACK);
        DrawRectangleLinesEx(tileBackground, 3, WHITE);

        // Title in large white text
        DrawText("Current Tiles:", tileDisplayX, tileDisplayY, 24, WHITE);

        // Draw tiles horizontally
        for (int i = 0; i < (int)mahjongTiles.tiles.size(); i++)
        {
            int tileX = tileDisplayX + 20 + i * 80; // Increased spacing
            int tileY = tileDisplayY + 35;

            // Draw tile background
            Rectangle tileBg = {(float)(tileX - 5), (float)(tileY - 5), 65.0f, 55.0f};
            const Tile& tile = mahjongTiles.tiles[i];

            Color bgColor;
            if (i == mahjongTiles.arrowPosition) {
                bgColor = RED; // Selected
            } else if (tile.isGold) {
                bgColor = GOLD; // KONG tiles are gold
            } else if (tile.IsZero()) {
                bgColor = {64, 64, 64, 255}; // Dark gray for zero tiles
            } else {
                bgColor = BLACK; // Normal tiles
            }

            // Border color based on tile type
            Color borderColor = WHITE;
            if (tile.IsZero()) {
                borderColor = {192, 192, 192, 255}; // Silver border for zero tiles
            } else if (tile.type == HAT_TILES) {
                borderColor = LIGHTBLUE;
            } else if (tile.type == DOT_TILES) {
                borderColor = LIGHTGREEN;
            }

            DrawRectangleRec(tileBg, bgColor);
            DrawRectangleLinesEx(tileBg, 3, borderColor);

            // Draw arrow pointer below tile pointing up - larger size
            if (i == mahjongTiles.arrowPosition)
            {
                DrawTriangle(
                    Vector2{(float)(tileX + 27), (float)(tileY + 55)}, // Top point
                    Vector2{(float)(tileX + 12), (float)(tileY + 75)}, // Bottom left
                    Vector2{(float)(tileX + 42), (float)(tileY + 75)}, // Bottom right
                    WHITE
                );
            }

            // Draw tile content based on type and gold status
            Color textColor = tile.isGold ? BLACK : WHITE; // Black text on gold tiles

            if (tile.IsZero()) {
                // Special display for zero tiles - larger and centered
                DrawText("0", tileX + 18, tileY + 5, 40, {192, 192, 192, 255}); 
            } else if (tile.type == PLAIN_TILES) {
                DrawText(TextFormat("%d", tile.value), tileX + 15, tileY + 5, 40, textColor);
            } else if (tile.type == HAT_TILES) {
                DrawText(TextFormat("%d^", tile.value), tileX + 10, tileY + 5, 36, textColor);
            } else if (tile.type == DOT_TILES) {
                DrawText(TextFormat("%d.", tile.value), tileX + 10, tileY + 5, 36, textColor);
            }
        }

        // Draw KEEP option
        int keepX = tileDisplayX + 20 + mahjongTiles.tiles.size() * 80;
        int keepY = tileDisplayY + 35;

        Rectangle keepBg = {(float)(keepX - 5), (float)(keepY - 5), 85.0f, 55.0f};
        Color keepBgColor = (mahjongTiles.arrowPosition == mahjongTiles.maxTiles) ? RED : BLACK;
        DrawRectangleRec(keepBg, keepBgColor);
        DrawRectangleLinesEx(keepBg, 3, WHITE);

        // Draw arrow pointer below KEEP pointing up - larger size
        if (mahjongTiles.arrowPosition == mahjongTiles.maxTiles)
        {
            DrawTriangle(
                Vector2{(float)(keepX + 37), (float)(keepY + 55)}, // Top point
                Vector2{(float)(keepX + 22), (float)(keepY + 75)}, // Bottom left
                Vector2{(float)(keepX + 52), (float)(keepY + 75)}, // Bottom right
                WHITE
            );
        }

        // Draw KEEP text
        DrawText("KEEP", keepX + 10, keepY + 10, 28, WHITE);
    }

    void DrawUpgradeTile()
    {
        // Get the color for this LOONG type
        Color upgradeColor = WHITE;
        string upgradeSymbol = "?";

        switch (upgradeTileType) {
            case BASIC_LOONG:
                upgradeColor = {34, 139, 34, 255}; // Forest Green
                upgradeSymbol = "B";
                break;
            case FIRE_LOONG:
                upgradeColor = {255, 69, 0, 255}; // Red Orange
                upgradeSymbol = "F";
                break;
            case WATER_LOONG:
                upgradeColor = {30, 144, 255, 255}; // Dodger Blue
                upgradeSymbol = "W";
                break;
            case WHITE_LOONG:
                upgradeColor = {248, 248, 255, 255}; // Ghost White
                upgradeSymbol = "P"; // P for Pure
                break;
            case EARTH_LOONG:
                upgradeColor = {139, 69, 19, 255}; // Saddle Brown
                upgradeSymbol = "E";
                break;
            case WIND_LOONG:
                upgradeColor = {192, 192, 192, 255}; // Silver
                upgradeSymbol = "A"; // A for Air
                break;
            case SHADOW_LOONG:
                upgradeColor = {75, 0, 130, 255}; // Indigo
                upgradeSymbol = "S";
                break;
            case CELESTIAL_LOONG:
                upgradeColor = {255, 215, 0, 255}; // Gold
                upgradeSymbol = "C";
                break;
        }

        // Draw upgrade tile with diamond/dragon symbol
        int tileX = gameAreaOffset + upgradeTilePosition.x * cellSize;
        int tileY = gameAreaOffset + upgradeTilePosition.y * cellSize;

        // Draw pulsing background
        static float pulseTimer = 0.0f;
        pulseTimer += GetFrameTime();
        float pulse = (sin(pulseTimer * 8) + 1) * 0.5f; // 0 to 1

        // Outer glow effect
        Rectangle glowRect = {(float)(tileX - 5), (float)(tileY - 5), (float)(cellSize + 10), (float)(cellSize + 10)};
        Color glowColor = upgradeColor;
        glowColor.a = (unsigned char)(100 + pulse * 100);
        DrawRectangleRec(glowRect, glowColor);

        // Main tile
        Rectangle tileRect = {(float)tileX, (float)tileY, (float)cellSize, (float)cellSize};
        DrawRectangleRec(tileRect, upgradeColor);
        DrawRectangleLinesEx(tileRect, 4, WHITE);

        // Draw diamond/dragon symbol in center
        int symbolSize = 32;
        int symbolX = tileX + cellSize/2 - MeasureText(upgradeSymbol.c_str(), symbolSize)/2;
        int symbolY = tileY + cellSize/2 - symbolSize/2;
        DrawText(upgradeSymbol.c_str(), symbolX, symbolY, symbolSize, BLACK);

        // Draw upgrade type name above tile
        string typeName = GetLoongTypeName(upgradeTileType) + " Upgrade";
        int nameWidth = MeasureText(typeName.c_str(), 16);
        DrawText(typeName.c_str(), tileX + cellSize/2 - nameWidth/2, tileY - 25, 16, upgradeColor);
    }

    void DrawUI()
    {
        // Draw black background for UI panel
        Rectangle uiBackground = {(float)(uiPanelX - 5), 0.0f, (float)(uiPanelWidth + 5), (float)canvasHeight};
        DrawRectangleRec(uiBackground, BLACK);
        DrawRectangleLinesEx(uiBackground, 3, WHITE);

        // Title and scores in large white text
        DrawText("Mahjong Loong", uiPanelX + 20, 30, 32, WHITE);
        DrawText(TextFormat("Score: %d", score), uiPanelX + 20, 70, 28, WHITE);

        // Show LOONG-specific high score
        pair<LoongType, DifficultyLevel> currentKey = {selectedLoongType, selectedDifficulty};
        int loongHigh = (loongHighScores.count(currentKey) > 0) ? loongHighScores[currentKey] : 0;
        DrawText(TextFormat("High: %d", max(highScore, loongHigh)), uiPanelX + 20, 105, 24, WHITE);

        // Health display with icons
        for (int i = 0; i < extraLives; i++) {
            DrawText("❤", uiPanelX + 20 + (i * 30), 140, 24, RED);
        }
        if (phoenixRebirthCharges > 0) {
            DrawText(TextFormat("+ %d Phoenix", phoenixRebirthCharges), uiPanelX + 20 + (extraLives * 30), 140, 20, GOLD);
        }

        // Difficulty and ornate level
        DrawText(TextFormat("Rank: %s", difficultyNames[selectedDifficulty].c_str()), uiPanelX + 20, 180, 18, YELLOW);
        DrawText(TextFormat("Level: %d", ornateLevel + 1), uiPanelX + 20, 205, 20, GOLD);

        // STRATEGIC NEXT TILE PREVIEW - Make this prominent!
        if (mahjongTiles.futureTilesLocked && mahjongTiles.lockedTilesRemaining > 0) {
            DrawText("LOCKED TILE:", uiPanelX + 20, 260, 22, GOLD);
        } else {
            DrawText("NEXT TILE:", uiPanelX + 20, 260, 22, GOLD);
        }

        string nextTileStr = mahjongTiles.nextTile.ToString();
        Color nextTileColor = WHITE;
        if (mahjongTiles.nextTile.type == HAT_TILES) nextTileColor = LIGHTBLUE;
        else if (mahjongTiles.nextTile.type == DOT_TILES) nextTileColor = LIGHTGREEN;

        // Special color for locked tiles
        if (mahjongTiles.futureTilesLocked && mahjongTiles.lockedTilesRemaining > 0) {
            nextTileColor = GOLD; // Golden color for locked tiles
        }

        // Draw next tile in a larger, more prominent box
        Rectangle nextTileBox = {(float)(uiPanelX + 20), 290, 80, 60};
        DrawRectangleRec(nextTileBox, BLACK);
        DrawRectangleLinesEx(nextTileBox, 4, nextTileColor); // Thicker border
        int nextTileWidth = MeasureText(nextTileStr.c_str(), 30);
        DrawText(nextTileStr.c_str(), uiPanelX + 60 - nextTileWidth/2, 305, 30, nextTileColor);

        // Strategic hint or locked tiles info
        if (mahjongTiles.futureTilesLocked && mahjongTiles.lockedTilesRemaining > 0) {
            char lockedInfo[50];
            sprintf(lockedInfo, "Locked tiles left: %d", mahjongTiles.lockedTilesRemaining);
            DrawText(lockedInfo, uiPanelX + 20, 380, 14, GOLD);
        } else {
            DrawText("Plan your moves!", uiPanelX + 20, 360, 14, YELLOW);
        }

        // SHIFT POWER DISPLAY - Below next tile preview
        DrawText("DRAGON POWER:", uiPanelX + 20, 400, 18, GOLD);

        // Power name and description - more compact
        DrawText(shiftPowerName.c_str(), uiPanelX + 20, 425, 16, WHITE);

        // Cooldown display - compact
        if (shiftPowerReady) {
            DrawText("READY! [SHIFT]", uiPanelX + 20, 450, 14, GREEN);
        } else {
            // Cooldown bar - smaller
            Rectangle cooldownBar = {(float)(uiPanelX + 20), 450, 150, 10};
            DrawRectangleRec(cooldownBar, DARKGRAY);
            float progress = (float)shiftCooldownTiles / (float)shiftCooldownMax;
            Rectangle progressBar = {(float)(uiPanelX + 20), 450, 150 * progress, 10};
            DrawRectangleRec(progressBar, GREEN);
        }

        // Evolution Progress
        DrawText(TextFormat("Next Evolution: %d/%d", snakeAge, nextUpgradeThreshold), uiPanelX + 20, 500, 16, YELLOW);
        Rectangle upgradeBg = {(float)(uiPanelX + 20), 525, 150, 8};
        DrawRectangleRec(upgradeBg, DARKGRAY);
        float upProgress = (float)snakeAge / (float)nextUpgradeThreshold;
        DrawRectangleRec({upgradeBg.x, upgradeBg.y, 150 * upProgress, 8}, GOLD);
    }

    void DrawChoiceWindow()
    {
        // Draw semi-transparent overlay
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {0, 0, 0, 180});

        // Draw choice window background
        int windowWidth = 600;
        int windowHeight = 400;
        int windowX = canvasWidth/2 - windowWidth/2;
        int windowY = canvasHeight/2 - windowHeight/2;

        Rectangle windowBg = {(float)windowX, (float)windowY, (float)windowWidth, (float)windowHeight};
        DrawRectangleRec(windowBg, BLACK);
        DrawRectangleLinesEx(windowBg, 5, WHITE);

        // Draw title
        int titleWidth = MeasureText("SNAKE EVOLUTION", 40);
        DrawText("LOONG EVOLUTION", canvasWidth/2 - titleWidth/2, windowY + 20, 40, GOLD);

        // Draw age info
        char ageText[50];
        sprintf(ageText, "Age %d - Choose your evolution:", snakeAge);
        int ageWidth = MeasureText(ageText, 24);
        DrawText(ageText, canvasWidth/2 - ageWidth/2, windowY + 70, 24, WHITE);

        // Draw instruction instead of timer
        int instWidth2 = MeasureText("Take your time to choose wisely", 18);
        DrawText("Take your time to choose wisely", canvasWidth/2 - instWidth2/2, windowY + 100, 18, YELLOW);

        // Draw choices
        for (int i = 0; i < 3; i++) {
            int choiceY = windowY + 120 + i * 80;
            Color choiceColor = (i == selectedChoice) ? GOLD : WHITE;
            Color bgColor = BLACK;
            if (i == selectedChoice) {
                bgColor.r = 50; bgColor.g = 50; bgColor.b = 50; bgColor.a = 255;
            } else {
                bgColor.r = 20; bgColor.g = 20; bgColor.b = 20; bgColor.a = 255;
            }

            // Draw choice background
            Rectangle choiceBg = {(float)(windowX + 20), (float)(choiceY - 5), (float)(windowWidth - 40), 70.0f};
            DrawRectangleRec(choiceBg, bgColor);
            DrawRectangleLinesEx(choiceBg, 2, choiceColor);

            // Draw arrow for selected choice
            if (i == selectedChoice) {
                DrawTriangle(
                    Vector2{(float)(windowX + 40), (float)(choiceY + 25)}, // Left point
                    Vector2{(float)(windowX + 55), (float)(choiceY + 15)}, // Top right
                    Vector2{(float)(windowX + 55), (float)(choiceY + 35)}, // Bottom right
                    GOLD
                );
            }

            // Draw choice name
            DrawText(currentChoices[i].c_str(), windowX + 70, choiceY, 24, choiceColor);

            // Draw choice description (pros and cons)
            DrawText(currentChoiceDescriptions[i].c_str(), windowX + 70, choiceY + 30, 16, GRAY);
        }

        // Draw instructions
        int instWidth = MeasureText("Hover to select, Left Click to confirm", 18);
        DrawText("Hover to select, Left Click to confirm",
                canvasWidth/2 - instWidth/2, windowY + windowHeight - 40, 18, LIGHTGRAY);
    }

    void DrawDebugUI()
    {
        // Draw debug panel on bottom-left for better visibility
        int debugHeight = 220;
        Rectangle debugBg = {10, (float)(canvasHeight - debugHeight - 10), 320, (float)debugHeight};
        DrawRectangleRec(debugBg, {0, 0, 0, 180});
        DrawRectangleLinesEx(debugBg, 2, YELLOW);

        int startY = canvasHeight - debugHeight;
        DrawText("DEBUG INFO", 20, startY, 20, YELLOW);

        // Age and Length tracking
        char ageText[50];
        sprintf(ageText, "Age: %d | Length: %d", snakeAge, (int)snake.body.size());
        DrawText(ageText, 20, startY + 25, 16, WHITE);

        // Speed multiplier
        char speedText[50];
        sprintf(speedText, "Speed Multiplier: %.1fx", currentSpeedMultiplier);
        DrawText(speedText, 20, startY + 45, 16, WHITE);

        // Probability bonus with legend
        char probText[50];
        sprintf(probText, "Probability Bonus: %.0f%%", currentProbabilityBonus);
        DrawText(probText, 20, startY + 65, 16, WHITE);

        if (currentProbabilityBonus > 0) {
            DrawText("🔵 Blue = Held tiles (KONG boost)", 20, startY + 80, 12, SKYBLUE);
            DrawText("🟡 Gold = Adjacent tiles (+/-1)", 20, startY + 95, 12, GOLD);
        }

        // REMOVED: Legacy auto-complete countdown
        int nextY = startY + 110;

        // Extra lives
        char livesText[50];
        sprintf(livesText, "Celestial: %d | Phoenix: %d | Basic: %d", celestialCharges, phoenixRebirthCharges, extraLives);
        DrawText(livesText, 20, nextY, 14, WHITE);
        nextY += 20;

        // Ornate level
        char ornateText[50];
        sprintf(ornateText, "Ornate Level: %d", ornateLevel + 1);
        DrawText(ornateText, 20, nextY, 16, WHITE);
        nextY += 20;

        // Current tile count
        char tileCountText[50];
        sprintf(tileCountText, "Tile Count: %d", currentTileCount);
        DrawText(tileCountText, 20, nextY, 16, WHITE);
        nextY += 20;

        // Active power-ups
        DrawText("Active Power-ups:", 20, nextY, 14, LIGHTGRAY);
        int yOffset = nextY + 15;
        if (slowGrowthActive) { DrawText("- Slow Growth", 20, yOffset, 12, GREEN); yOffset += 15; }
        if (speedDemonActive) { DrawText("- Speed Demon", 20, yOffset, 12, GREEN); yOffset += 15; }
        if (turtleModeActive) { DrawText("- Turtle Mode", 20, yOffset, 12, GREEN); yOffset += 15; }
        if (survivorActive) { DrawText("- Survivor", 20, yOffset, 12, GREEN); yOffset += 15; }
        if (luckyNumbersActive) { DrawText("- Lucky Numbers", 20, yOffset, 12, GREEN); yOffset += 15; }

        // Music debug with safety checks
        char musicText[50];
        int currentLevel = ornateLevel + 1;
        if (currentLevel < 1) currentLevel = 1;
        if (currentLevel > 10) currentLevel = 10;

        string currentMusicName = "unknown";
        if (musicMap.find(currentLevel) != musicMap.end()) {
            currentMusicName = musicMap[currentLevel];
        }
        sprintf(musicText, "Music: %s (L%d)", currentMusicName.c_str(), currentLevel);
        DrawText(musicText, 20, startY + 200, 12, GOLD);
    }

    void DrawExtraLifeOverlay()
    {
        // Draw different overlay colors based on extra life type
        if (lastExtraLifeType == "Special") {
            DrawRectangle(0, 0, canvasWidth, canvasHeight, {100, 0, 255, 120}); // Purple for special
        } else {
            DrawRectangle(0, 0, canvasWidth, canvasHeight, {255, 0, 0, 100}); // Red for basic
        }

        // Draw different messages based on extra life type
        if (lastExtraLifeType == "Special") {
            int msgWidth = MeasureText("🐉 SPECIAL EXTRA LIFE! 🐉", 48);
            DrawText("🐉 SPECIAL EXTRA LIFE! 🐉", canvasWidth/2 - msgWidth/2, canvasHeight/2 - 80, 48, GOLD);

            int effectWidth = MeasureText("LOONG Power Activated!", 32);
            DrawText("LOONG Power Activated!", canvasWidth/2 - effectWidth/2, canvasWidth/2 - 40, 32, WHITE);
        } else {
            int msgWidth = MeasureText("BASIC EXTRA LIFE", 60);
            DrawText("BASIC EXTRA LIFE", canvasWidth/2 - msgWidth/2, canvasHeight/2 - 60, 60, WHITE);
        }

        int instWidth = MeasureText("Move mouse to reposition, then LEFT CLICK to continue", 24);
        DrawText("Move mouse to reposition, then LEFT CLICK to continue",
                canvasWidth/2 - instWidth/2, canvasHeight/2 + 20, 24, YELLOW);

        // Show both types of lives remaining
        char livesText[100];
        sprintf(livesText, "Celestial: %d | Phoenix: %d | Basic: %d", celestialCharges, phoenixRebirthCharges, extraLives);
        int livesWidth = MeasureText(livesText, 24);
        DrawText(livesText, canvasWidth/2 - livesWidth/2, canvasHeight/2 + 60, 24, WHITE);
    }

    void DrawPhoenixRebirthEffect()
    {
        // Draw epic phoenix rebirth effect
        static float effectTimer = 0.0f;
        effectTimer += GetFrameTime();

        // Pulsing fire overlay
        float alpha = (sin(effectTimer * 10) + 1) * 0.5f * 150; // Pulsing between 0-150
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {255, 100, 0, (unsigned char)alpha});

        // Phoenix rebirth message with fire colors
        int msgWidth = MeasureText("🔥🐦 PHOENIX REBIRTH! 🐦🔥", 48);
        Color fireColor = {255, (unsigned char)(150 + sin(effectTimer * 8) * 50), 0, 255};
        DrawText("🔥🐦 PHOENIX REBIRTH! 🐦🔥", canvasWidth/2 - msgWidth/2, canvasHeight/2 - 40, 48, fireColor);

        // Charges remaining
        char chargesText[50];
        sprintf(chargesText, "Phoenix Charges Remaining: %d", phoenixCharges);
        int chargesWidth = MeasureText(chargesText, 24);
        DrawText(chargesText, canvasWidth/2 - chargesWidth/2, canvasHeight/2 + 20, 24, GOLD);

        // Reset timer after 3 seconds
        if (effectTimer > 3.0f) {
            effectTimer = 0.0f;
        }
    }

    void DrawCosmicWisdomEffect()
    {
        // Draw cosmic auto-complete effect
        static float cosmicTimer = 0.0f;
        cosmicTimer += GetFrameTime();

        // Cosmic blue/purple overlay
        float alpha = (sin(cosmicTimer * 12) + 1) * 0.5f * 120; // Pulsing cosmic effect
        DrawRectangle(0, 0, canvasWidth, canvasHeight, {100, 0, 255, (unsigned char)alpha});

        // Cosmic wisdom message
        int msgWidth = MeasureText("🌌✨ COSMIC WISDOM! ✨🌌", 48);
        Color cosmicColor = {(unsigned char)(150 + sin(cosmicTimer * 6) * 50), 100, 255, 255};
        DrawText("🌌✨ COSMIC WISDOM! ✨🌌", canvasWidth/2 - msgWidth/2, canvasHeight/2 - 40, 48, cosmicColor);

        // Auto-complete message
        int autoWidth = MeasureText("Hand Auto-Completed!", 24);
        DrawText("Hand Auto-Completed!", canvasWidth/2 - autoWidth/2, canvasHeight/2 + 20, 24, WHITE);

        // Reset timer after 2 seconds
        if (cosmicTimer > 2.0f) {
            cosmicTimer = 0.0f;
        }
    }

    void CheckCollisionWithFood()
    {
        // Check collision with upgrade tile first
        if (upgradeSpawned && Vector2Equals(snake.body[0], upgradeTilePosition)) {
            CollectUpgradeTile();
            return; // Don't process food collision this frame
        }

        // Check collision with latent cultivation tile
        if (latentUpgradeSpawned && Vector2Equals(snake.body[0], latentUpgradeTilePosition)) {
            CollectLatentUpgradeTile();
            return; // Don't process food collision this frame
        }

        if (Vector2Equals(snake.body[0], food.position))
        {
            // Increase snake age and fruit counter
            snakeAge++;
            fruitCounter++;

            // Use the next tile (strategic preview system)
            Tile newTile = mahjongTiles.nextTile;

            // Generate the next preview tile for next fruit
            mahjongTiles.GenerateNextTile(currentProbabilityBonus);

            // Show number popup near snake head
            Vector2 headPos = {
                gameAreaOffset + snake.body[0].x * cellSize + cellSize / 2,
                gameAreaOffset + snake.body[0].y * cellSize + cellSize / 2
            };
            numberPopup.Show(newTile, headPos); // Show the full tile for popup

            // Check for KONG condition first
            if (mahjongTiles.CheckKongCondition(newTile))
            {
                // KONG condition met! Show celebration
                showKongWin = true;
                kongWinTimer = 0.8f; // Show briefly

                // Play triumphant "Kong" sound
                SetSoundPitch(eatSound, 1.8f);
                PlaySound(eatSound);

                // Apply power-up effects to scoring - KONG gives HALF of Mahjong score
                int kongPoints = (mahjongTiles.tiles.size() * 5) / 2; // Half of Mahjong score

                // Apply old evolution system multipliers
                if (gamblerActive) kongPoints *= 2;
                if (speedsterActive) kongPoints *= 3;
                if (survivorActive || monkActive) kongPoints = 0; // No bonuses

                UpdateLoongTotalScore(kongPoints); // Update latent upgrade progress

                // Apply LOONG upgrade multipliers! 🐉✨
                if (kongPoints > 0) {
                    // WHITE LOONG Sacred KONG: Check for four 0 KONG (200% bonus)
                    if (selectedLoongType == WHITE_LOONG && newTile.IsZero()) {
                        // Count zeros in hand
                        int zeroCount = 0;
                        for (const Tile& tile : mahjongTiles.tiles) {
                            if (tile.IsZero()) zeroCount++;
                        }

                        if (zeroCount >= 3) { // 3 in hand + 1 new = 4 zeros
                            kongPoints = (int)(kongPoints * 3.0f); // 200% bonus = 3x total
                            cout << "🤍 SACRED KONG: Four 0 KONG! 200% bonus! " << kongPoints << " points!" << endl;
                        }
                    }

                    kongPoints = (int)(kongPoints * kongScoreMultiplier);
                    cout << "🐉 LOONG KONG Bonus: " << kongPoints << " points (x" << kongScoreMultiplier << " multiplier)!" << endl;
                }

                score += kongPoints;
                UpdateLoongTotalScore(kongPoints); // Update latent upgrade progress
                if (score > highScore)
                {
                    highScore = score;
                }

                // KONG does NOT give instant win - continue playing with remaining tiles
                // The KONG tiles are already marked as gold and removed from pool
                cout << "KONG scored " << kongPoints << " points. Continue playing with remaining tiles." << endl;
            }
            // Check if this creates a winning combination
            else if (mahjongTiles.CheckWinCondition(newTile, ornateLevel))
            {
                // Check if this is a LOONG win (1-9 consecutive of same type)
                vector<Tile> testTiles = mahjongTiles.tiles;
                testTiles.push_back(newTile);
                mahjongTiles.SortTilesVector(testTiles);

                if (ornateLevel >= 2 && mahjongTiles.HasLoongWin(testTiles) && mahjongWinsAchieved >= mahjongWinsRequired) {
                    // LOONG WIN! Apply LOONG multiplier and go to cultivation success
                    cout << "LOONG WIN ACHIEVED! CULTIVATION SUCCESS!" << endl;

                    // Apply LOONG win multiplier (default 3x, can be upgraded to 4x, 5x, 6x)
                    score *= loongWinMultiplier;
                    cout << "🐉 LOONG WIN MULTIPLIER: " << loongWinMultiplier << "x! Final score: " << score << "!" << endl;
                    if (score > highScore) {
                        highScore = score;
                        SaveHighScore();
                    }

                    // Update LOONG-specific high score
                    pair<LoongType, DifficultyLevel> currentKey = {selectedLoongType, selectedDifficulty};
                    if (score > loongHighScores[currentKey]) {
                        loongHighScores[currentKey] = score;
                        cout << "New LOONG high score for " << availableLOONGs[selectedLoongIndex].name
                             << " (" << difficultyNames[selectedDifficulty] << "): " << score << endl;
                    }

                    // Unlock next difficulty since we achieved LOONG
                    UnlockNextDifficulty();

                    // Go to cultivation success screen
                    gameState = CULTIVATION_SUCCESS;
                    PlayTriumphantMahjongSound(mahjongWinSound);
                    return; // Exit early, don't continue normal game
                }

                // Normal Mahjong win
                showMahjongWin = true;
                mahjongWinTimer = 0.8f; // Show briefly

                // Increase mahjong wins and ornate level (simplified to 0-3)
                mahjongWins++;
                mahjongWinsAchieved++; // Track for difficulty system
                int previousOrnateLevel = ornateLevel;
                ornateLevel = min(mahjongWins, 3); // 1st win = level 1, 2nd = level 2, 3rd = level 3

                // Show LOONG popup when reaching level 3 for the first time
                if (ornateLevel == 3 && previousOrnateLevel < 3) {
                    cout << "REACHED FINAL LEVEL! LOONG WIN CONDITION UNLOCKED!" << endl;
                    // The popup will be shown in the UI
                }

                // Expand tiles after every Mahjong win
                ExpandTiles();

                // Note: RedrawCompleteHand() already sorts the hand automatically

                // Play triumphant "Mahjong" sound
                PlayTriumphantMahjongSound(mahjongWinSound);

                // Apply power-up effects to scoring - new formula: tiles * 5
                int mahjongPoints = mahjongTiles.tiles.size() * 5; // 4 tiles * 5 = 20 points

                // Apply old evolution system multipliers
                if (gamblerActive) mahjongPoints *= 2;
                if (speedsterActive) mahjongPoints *= 3;
                if (survivorActive || monkActive) mahjongPoints = 0; // No bonuses

                UpdateLoongTotalScore(mahjongPoints); // Update latent upgrade progress

                // Apply LOONG upgrade multipliers! 🐉✨
                if (mahjongPoints > 0) {
                    mahjongPoints = (int)(mahjongPoints * mahjongScoreMultiplier);

                    // Check for Shadow Clone effect
                    if (shadowCloneCharges > 0) {
                        shadowCloneCharges--;
                        mahjongPoints *= 2; // Double the points
                        cout << "👥 SHADOW CLONE ACTIVATED! Doubling Mahjong score! Charges remaining: " << shadowCloneCharges << endl;
                    }

                    cout << "🐉 LOONG Mahjong Bonus: " << mahjongPoints << " points (x" << mahjongScoreMultiplier << " multiplier)!" << endl;
                }

                score += mahjongPoints;
                UpdateLoongTotalScore(mahjongPoints); // Update latent upgrade progress
                if (score > highScore)
                {
                    highScore = score;
                }

                // Apply power-up effects to length
                if (!mahjongNerfActive && !survivorActive && !monkActive) {
                    int lengthToAdd = 5;
                    if (perfectionistActive) lengthToAdd = 8;
                    snake.AddSegments(lengthToAdd);
                }

                // WHITE LOONG Pure Mahjong: Turn tiles to 0 after Mahjong
                if (selectedLoongType == WHITE_LOONG) {
                    // Get Pure Mahjong level (upgrade index 0)
                    int pureMahjongLevel = 0;
                    for (const LoongData& loong : availableLOONGs) {
                        if (loong.type == WHITE_LOONG) {
                            pureMahjongLevel = loong.upgrades[0].level;
                            break;
                        }
                    }

                    // Turn 1->2->3 tiles to 0 based on upgrade level
                    if (pureMahjongLevel > 0) {
                        int tilesToConvert = min(pureMahjongLevel, (int)mahjongTiles.tiles.size());
                        for (int i = 0; i < tilesToConvert; i++) {
                            mahjongTiles.tiles[i] = Tile(0, PLAIN_TILES, false); // Convert to zero
                        }
                        mahjongTiles.SortTiles();
                        cout << ">>> PURE MAHJONG: Converted " << tilesToConvert << " tiles to 0!" << endl;
                    }
                }

                // CRITICAL FIX: Generate new next tile without resetting hand
                mahjongTiles.GenerateNextTile(currentProbabilityBonus);
            }
            else
            {
                // Normal eat sound - ensure normal pitch (optimized)
                PlaySoundSafe(eatSound, 1.0f, GetTime(), lastSoundTime, soundCooldown);

                // Check if KEEP is selected
                if (mahjongTiles.IsKeepSelected())
                {
                    // KEEP at final level: ONLY check for LOONG wins, never Mahjong
                    if (ornateLevel >= LEVEL_4_DRAGON) {
                        // At final level, KEEP only allows LOONG wins (risk-reward dynamic)
                        vector<Tile> testTiles = mahjongTiles.tiles;
                        testTiles.push_back(newTile);
                        mahjongTiles.SortTilesVector(testTiles);

                        if (mahjongTiles.HasLoongWin(testTiles) && mahjongWinsAchieved >= mahjongWinsRequired) {
                            // LOONG WIN! Apply LOONG multiplier and go to cultivation success
                            cout << "LOONG WIN ACHIEVED WITH KEEP! CULTIVATION SUCCESS!" << endl;

                            // Apply LOONG win multiplier (default 3x, can be upgraded to 4x, 5x, 6x)
                            score *= loongWinMultiplier;
                            cout << "🐉 LOONG WIN MULTIPLIER: " << loongWinMultiplier << "x! Final score: " << score << "!" << endl;
                            if (score > highScore) {
                                highScore = score;
                                SaveHighScore();
                            }

                            // Update LOONG-specific high score
                            pair<LoongType, DifficultyLevel> currentKey = {selectedLoongType, selectedDifficulty};
                            if (score > loongHighScores[currentKey]) {
                                loongHighScores[currentKey] = score;
                                cout << "New LOONG high score for " << availableLOONGs[selectedLoongIndex].name
                                     << " (" << difficultyNames[selectedDifficulty] << "): " << score << endl;
                            }

                            // Unlock next difficulty since we achieved LOONG
                            UnlockNextDifficulty();

                            // Go to cultivation success screen
                            gameState = CULTIVATION_SUCCESS;
                            PlayTriumphantMahjongSound(mahjongWinSound);
                            return; // Exit early, don't continue normal game
                        }

                        // KEEP at final level: No Mahjong check, just discard tile
                        cout << "KEEP at final level - aiming for LOONG only! Tile discarded." << endl;
                    }

                    // Apply growth effects
                    ApplyGrowthEffects();
                }
                else
                {
                    // Replace tile at arrow position
                    mahjongTiles.ReplaceTileAtArrow(newTile);
                    ApplyGrowthEffects();
                }

                // Apply fruit-based power-up effects
                ApplyFruitEffects();

                // Add fruit scoring - +1 point per fruit
                score += 1;
                UpdateLoongTotalScore(1); // Update latent upgrade progress
                if (score > highScore)
                {
                    highScore = score;
                }
            }

            // OLD AUTOMATIC UPGRADE SYSTEM REMOVED
            // Now using physical upgrade tiles that must be collected!

            // OLD EVOLUTION SYSTEM REMOVED
            // Now using upgrade tile system for all progression!

            // Generate new food position
            food.position = food.GenerateRandomPos(snake.body);

            // REMOVED: Legacy Celestial LOONG auto-complete ability
            // Now only available as SHIFT power

            // Check for Earth LOONG Earthquake auto-shuffle
            earthquakeCounter++;
            if (earthquakeCounter >= earthquakeThreshold) {
                cout << "🌍 EARTHQUAKE! Auto-shuffling tiles after " << earthquakeThreshold << " fruits!" << endl;
                earthquakeCounter = 0;

                // CRITICAL FIX: Shuffle existing hand instead of regenerating
                mahjongTiles.ShuffleExistingHand();

                // Play earthquake sound (optimized)
                PlaySoundSafe(eatSound, 0.5f, GetTime(), lastSoundTime, soundCooldown);
            }

            // Check for Wind LOONG Tornado tile swaps
            tornadoCounter++;
            if (tornadoCounter >= tornadoThreshold) {
                cout << "🌪️ TORNADO! Swapping 2 random tiles after " << tornadoThreshold << " fruits!" << endl;
                tornadoCounter = 0;

                // Swap 2 random tiles in hand
                if (mahjongTiles.tiles.size() >= 2) {
                    int idx1 = GetRandomValue(0, mahjongTiles.tiles.size() - 1);
                    int idx2 = GetRandomValue(0, mahjongTiles.tiles.size() - 1);
                    if (idx1 != idx2) {
                        swap(mahjongTiles.tiles[idx1], mahjongTiles.tiles[idx2]);
                        mahjongTiles.SortTiles();
                    }
                }

                // Play tornado sound (optimized)
                PlaySoundSafe(eatSound, 1.8f, GetTime(), lastSoundTime, soundCooldown);
            }

            // Check for Water LOONG Healing Waters
            healingCounter++;
            if (healingCounter >= 5 && waterHealingAmount > 0) { // Every 5 fruits
                cout << "💧 HEALING WATERS! Reducing length by " << waterHealingAmount << "!" << endl;
                healingCounter = 0;

                // Reduce snake length (healing effect)
                for (int i = 0; i < waterHealingAmount; i++) {
                    if (snake.body.size() > 6) {
                        snake.body.pop_back();
                    }
                }

                // Play healing sound (optimized)
                PlaySoundSafe(eatSound, 1.3f, GetTime(), lastSoundTime, soundCooldown);
            }

            // Check for Wind LOONG Lightning Speed length reduction
            if (fruitCounter % 5 == 0 && windLengthReduction > 0) {
                cout << "🌪️ LIGHTNING SPEED! Reducing length by " << windLengthReduction << "!" << endl;
                for (int i = 0; i < windLengthReduction; i++) {
                    if (snake.body.size() > 6) {
                        snake.body.pop_back();
                    }
                }
            }
        }
    }

    void CheckCollisionWithEdges()
    {
        if (snake.body[0].x == cellCount || snake.body[0].x == -1 ||
            snake.body[0].y == cellCount || snake.body[0].y == -1)
        {
            // Move head back to previous valid position immediately by shrinking
            if (snake.body.size() > 1) {
                snake.body.pop_front();
            }
            snake.direction = {0, 0}; // Hard stop: prevent tunneling through wall

            // Check for wall immunity (Water LOONG Tsunami Shield or Earth/White LOONG Granite Will)
            if (wallImmunities > 0) {
                wallImmunities--;

                if (isGraniteWillActive) {
                    cout << ">>> GRANITE WILL IMMUNITY: Stopped at wall! Immunities remaining: " << wallImmunities << endl;
                } else {
                    cout << ">>> TSUNAMI SHIELD ACTIVATED! Wall collision ignored. Remaining immunities: " << wallImmunities << endl;

                    // Teleport snake to safe position (center of screen)
                    snake.body[0].x = cellCount / 2;
                    snake.body[0].y = cellCount / 2;
                }

                // Play special immunity sound (optimized)
                PlaySoundSafe(eatSound, 0.8f, GetTime(), lastSoundTime, soundCooldown);

                return; // Don't die
            }

            HandleDeath();
        }
    }

    void HandleDeath()
    {
        // Check for Phoenix Rebirth (Fire LOONG ability)
        if (phoenixRebirth && phoenixCharges > 0) {
            phoenixCharges--;
            cout << "🔥🐦 PHOENIX REBIRTH ACTIVATED! Reviving with full score! Charges remaining: " << phoenixCharges << endl;

            // Keep current score (don't reset)
            // Reset snake position
            snake.body[0] = {(float)(cellCount/2), (float)(cellCount/2)};
            isInExtraLifeMode = true;

            // Show epic phoenix rebirth effect
            showPhoenixRebirth = true;
            phoenixRebirthTimer = 3.0f;
            PlaySoundSafe(eatSound, 2.0f, GetTime(), lastSoundTime, 0.0f);

            return; // Don't continue to normal death handling
        }

        // Use Celestial Rebirth first (normal revival like Bamboo)
        if (celestialRebirth && celestialCharges > 0) {
            celestialCharges--;
            isInExtraLifeMode = true;
            lastExtraLifeType = "Celestial";
            cout << ">>> CELESTIAL REBIRTH ACTIVATED! Normal revival! Charges remaining: " << celestialCharges << endl;
            ApplySpecialExtraLifeEffect();
            PlaySoundSafe(eatSound, 1.0f, GetTime(), lastSoundTime, soundCooldown); // Different sound for extra life
        }
        // Use Phoenix Rebirth second (with bonus scoring + speed penalty)
        else if (phoenixRebirthCharges > 0) {
            phoenixRebirthCharges--;
            isInExtraLifeMode = true;
            lastExtraLifeType = "Phoenix";

            // PHOENIX REBIRTH BONUS SCORING - Add % of current score
            int rebirthBonus = score * 0.25f; // 25% of current score
            score += rebirthBonus;

            // PHOENIX REBIRTH SPEED PENALTY - Boost speed as penalty for power
            currentSpeedMultiplier *= 1.5f; // 50% faster (penalty)
            cout << ">>> PHOENIX REBIRTH ACTIVATED! Bonus: " << rebirthBonus << " points (25% of score)! Speed boosted to " << (currentSpeedMultiplier * 100) << "%! Charges remaining: " << phoenixRebirthCharges << endl;
            ApplySpecialExtraLifeEffect();
            PlaySoundSafe(eatSound, 1.0f, GetTime(), lastSoundTime, soundCooldown); // Different sound for extra life
        }
        // Then use regular extra lives (basic effect)
        else if (extraLives > 0) {
            extraLives--;
            isInExtraLifeMode = true;
            lastExtraLifeType = "Basic";
            cout << "Basic extra life used! Lives remaining: " << extraLives << endl;
            PlaySoundSafe(eatSound, 1.0f, GetTime(), lastSoundTime, soundCooldown); // Different sound for extra life
        } else {
            // WHITE LOONG Divine Shield: Remove zeros instead of game over
            if (selectedLoongType == WHITE_LOONG && divineShieldZeros > 0) {
                // Count zeros in hand
                int zerosInHand = 0;
                for (const Tile& tile : mahjongTiles.tiles) {
                    if (tile.IsZero()) zerosInHand++;
                }

                if (zerosInHand > 0) {
                    // Remove up to divineShieldZeros zeros from hand
                    int zerosToRemove = min(divineShieldZeros, zerosInHand);
                    int zerosRemoved = 0;

                    for (int i = 0; i < (int)mahjongTiles.tiles.size() && zerosRemoved < zerosToRemove; i++) {
                        if (mahjongTiles.tiles[i].IsZero()) {
                            mahjongTiles.tiles.erase(mahjongTiles.tiles.begin() + i);
                            zerosRemoved++;
                            i--; // Adjust index after removal
                        }
                    }
                    isInExtraLifeMode = true;
                    lastExtraLifeType = "Divine Shield";

                    cout << "🤍 DIVINE SHIELD ACTIVATED! Removed " << zerosRemoved << " zeros from hand! Continue playing!" << endl;

                    // Play special divine sound (optimized)
                    PlaySoundSafe(eatSound, 1.8f, GetTime(), lastSoundTime, soundCooldown);

                    return; // Don't game over
                }
            }

            // No extra lives or divine shield, normal game over
            GameOver();
        }
    }

    void ApplySpecialExtraLifeEffect() {
        // Apply unique effects based on selected LOONG type
        switch (selectedLoongType) {
            case BASIC_LOONG:
                // Basic LOONG: Just basic extra life (no special effect)
                cout << "🐉 Basic LOONG Extra Life: Standard revival!" << endl;
                break;

            case FIRE_LOONG:
                // Fire LOONG: If have triple, immediately KONG and remove from pool
                cout << "🔥 Fire LOONG Extra Life: Blazing Revival!" << endl;
                for (int value = 1; value <= 9; value++) {
                    for (TileType type : mahjongTiles.availableTypes) {
                        pair<int, TileType> tileKey = {value, type};
                        int count = 0;
                        for (const Tile& tile : mahjongTiles.tiles) {
                            if (tile.value == value && tile.type == type) count++;
                        }
                        if (count >= 3) {
                            // Auto-KONG this tile
                            mahjongTiles.goldTiles.insert(tileKey);
                            cout << "🔥 Auto-KONG: " << value << " removed from pool!" << endl;
                            break;
                        }
                    }
                }
                break;

            case WATER_LOONG:
                // Water LOONG: Reduce speed by 50% until eaten 10 tiles
                cout << "💧 Water LOONG Extra Life: Flowing Revival!" << endl;
                currentSpeedMultiplier *= 0.5f;
                // TODO: Add counter for 10 tiles to restore speed
                break;

            case SHADOW_LOONG:
                // Shadow LOONG: Can cross over own body 3-6-9 times
                cout << "🌑 Shadow LOONG Extra Life: Shadow Revival!" << endl;
                teleportCharges += 3; // Add shadow teleport charges
                break;

            default:
                cout << "🐉 Special Extra Life: Mystical revival!" << endl;
                break;
        }
    }

    void GameOver()
    {
        // Update global high score
        if (score > highScore)
        {
            highScore = score;
            SaveHighScore();
        }

        // Update LOONG-specific high score
        pair<LoongType, DifficultyLevel> currentKey = {selectedLoongType, selectedDifficulty};
        if (score > loongHighScores[currentKey]) {
            loongHighScores[currentKey] = score;
            cout << "New high score for " << availableLOONGs[selectedLoongIndex].name
                 << " (" << difficultyNames[selectedDifficulty] << "): " << score << endl;
        }

        // Check if we achieved LOONG (reached ornate level 4) to unlock next difficulty
        if (ornateLevel >= LEVEL_4_DRAGON) {
            UnlockNextDifficulty();
        }

        // Save progress after every game
        SaveProgressData();

        // Store final score before resetting
        finalScore = score;

        snake.Reset();
        food.position = food.GenerateRandomPos(snake.body);
        cout << "🎮 STARTING NEW GAME with selectedLatentLevel: " << selectedLatentLevel << endl;
        mahjongTiles.GenerateRandomTiles(selectedLatentLevel);

        // Reset ornate level and mahjong wins on death
        ornateLevel = LEVEL_1_NONE;
        mahjongWins = 0;
        mahjongWinsAchieved = 0; // Reset difficulty tracking

        // Reset age and choice systems
        snakeAge = 0;
        snakeLength = snake.body.size(); // Initialize with starting snake length
        showChoiceWindow = false;
        showKongWin = false;
        showLoongUpgrade = false;
        fruitCounter = 0;

        // Reset LOONG upgrades (keep selected LOONG but reset temporary effects)
        currentSpeedMultiplier = 1.0f;
        currentProbabilityBonus = 0.0f;

        // Reset LOONG effect variables
        mahjongScoreMultiplier = 1.0f;
        kongScoreMultiplier = 1.0f;
        wallImmunities = 0;
        teleportCharges = 0;
        // REMOVED: Legacy auto-complete system
        loongWinMultiplier = 3;
        phoenixRebirth = false;
        phoenixCharges = 0;
        celestialRebirth = false;
        celestialCharges = 0;

        // Reset zero tile system
        purificationCounter = 0;
        purificationThreshold = 15;
        divineShieldZeros = 3;

        // Reset color mixing system
        mixedLoongTypes.clear();

        // Reset advanced LOONG effects
        shadowCloneCharges = 0;
        earthquakeCounter = 0;
        earthquakeThreshold = 10;
        tornadoCounter = 0;
        tornadoThreshold = 7;
        healingCounter = 0;
        graniteWillActive = false;

        // Reset upgrade tile system
        upgradeSpawned = false;
        tilesConsumedSinceUpgrade = 0;
        nextUpgradeThreshold = 10;
        isGraniteWillActive = false;

        // Reset tile locking system
        mahjongTiles.futureTilesLocked = false;
        mahjongTiles.lockedTilesRemaining = 0;
        mahjongTiles.lockedFutureTiles.clear();

        // Reset all power-up effects
        slowGrowthActive = false;
        mahjongNerfActive = false;
        speedDemonActive = false;
        luckyNumbersActive = false;
        minimalistActive = false;
        turtleModeActive = false;
        gamblerActive = false;
        perfectionistActive = false;
        collectorActive = false;
        survivorActive = false;
        speedsterActive = false;
        monkActive = false;
        extraLives = 0;
        phoenixRebirthCharges = 0;

        // Reset debug variables
        currentSpeedMultiplier = 1.0f;
        currentProbabilityBonus = 0.0f;
        isInExtraLifeMode = false;
        lastExtraLifeType = "";
        lastOrnateLevel = -1; // Reset music tracking

        // Reset kong wins and tile system
        kongWins = 0;
        totalWins = 0;
        currentTileCount = 4;
        waterHealingAmount = 0;
        windLengthReduction = 0;

        // Reset tile system completely
        mahjongTiles.availableTypes.clear();
        mahjongTiles.availableTypes.push_back(PLAIN_TILES); // Back to plain tiles only
        mahjongTiles.goldTiles.clear(); // Clear gold tiles
        mahjongTiles.tileCounts.clear(); // Clear tile counts

        // Reinitialize tile counts
        for (int i = 1; i <= 9; i++) {
            mahjongTiles.tileCounts[make_pair(i, PLAIN_TILES)] = 0;
            mahjongTiles.tileCounts[make_pair(i, HAT_TILES)] = 0;
            mahjongTiles.tileCounts[make_pair(i, DOT_TILES)] = 0;
        }

        mahjongTiles.SetTileCount(4, 0); // Reset to 4 tiles, ornate level 0

        gameState = GAME_OVER;
        score = 0; // Reset for next game
        PlaySound(wallSound);

        // Select and play random game over music
        if (GetRandomValue(0, 1) == 0) {
            currentGameOverMusic = gameOverMusic1;
            cout << "Selected game_over_eng.mp3 for game over" << endl;
        } else {
            currentGameOverMusic = gameOverMusic2;
            cout << "Selected game_over_cn.mp3 for game over" << endl;
        }

        // Stop all other music and play game over music
        if (IsMusicStreamPlaying(backgroundMusic)) StopMusicStream(backgroundMusic);
        if (IsMusicStreamPlaying(alternateMusic)) StopMusicStream(alternateMusic);
        if (IsMusicStreamPlaying(titleScreenMusic)) StopMusicStream(titleScreenMusic);
        if (IsMusicStreamPlaying(loongSelectMusic)) StopMusicStream(loongSelectMusic);

        if (currentGameOverMusic.stream.buffer != NULL) {
            PlayMusicStream(currentGameOverMusic);
            cout << "Started playing game over music" << endl;
        }
    }

    void ResetGame() {
        // Reset game state for new game (similar to HandleDeath but without GAME_OVER)
        snake.Reset();
        food.position = food.GenerateRandomPos(snake.body);
        cout << "🔄 RESETTING GAME with selectedLatentLevel: " << selectedLatentLevel << endl;
        mahjongTiles.GenerateRandomTiles(0);

        // Reset ornate level and mahjong wins
        ornateLevel = LEVEL_1_NONE;
        mahjongWins = 0;
        mahjongWinsAchieved = 0; // Reset difficulty tracking

        // Reset age and choice systems
        snakeAge = 0;
        snakeLength = snake.body.size(); // Initialize with starting snake length
        showChoiceWindow = false;
        showKongWin = false;
        showLoongUpgrade = false;
        showPhoenixRebirth = false;
        showCosmicWisdom = false;
        fruitCounter = 0;

        // Reset LOONG upgrades (keep selected LOONG but reset temporary effects)
        currentSpeedMultiplier = 1.0f;
        currentProbabilityBonus = 0.0f;

        // Reset LOONG effect variables
        mahjongScoreMultiplier = 1.0f;
        kongScoreMultiplier = 1.0f;
        wallImmunities = 0;
        teleportCharges = 0;
        // REMOVED: Legacy auto-complete system
        loongWinMultiplier = 3;
        phoenixRebirth = false;
        phoenixCharges = 0;
        celestialRebirth = false;
        celestialCharges = 0;

        // Reset zero tile system
        purificationCounter = 0;
        purificationThreshold = 15;
        divineShieldZeros = 3;

        // Reset color mixing system
        mixedLoongTypes.clear();

        // Reset advanced LOONG effects
        shadowCloneCharges = 0;
        earthquakeCounter = 0;
        earthquakeThreshold = 10;
        tornadoCounter = 0;
        tornadoThreshold = 7;
        healingCounter = 0;
        graniteWillActive = false;

        // Reset upgrade tile system
        upgradeSpawned = false;
        tilesConsumedSinceUpgrade = 0;
        nextUpgradeThreshold = 10;
        isGraniteWillActive = false;

        // Reset latent cultivation spawning
        latentCultivationSpawned = false;
        pendingLatentUpgrades.clear();
        latentUpgradeSpawnTimer = 0.0f;
        latentUpgradeSpawned = false;

        // Reset tile locking system
        mahjongTiles.futureTilesLocked = false;
        mahjongTiles.lockedTilesRemaining = 0;
        mahjongTiles.lockedFutureTiles.clear();

        // Reset all power-up effects
        slowGrowthActive = false;
        mahjongNerfActive = false;
        speedDemonActive = false;
        luckyNumbersActive = false;
        minimalistActive = false;
        turtleModeActive = false;
        gamblerActive = false;
        perfectionistActive = false;
        collectorActive = false;
        survivorActive = false;
        speedsterActive = false;
        monkActive = false;
        extraLives = 0;
        phoenixRebirthCharges = 0;

        // Reset debug variables
        currentSpeedMultiplier = 1.0f;
        currentProbabilityBonus = 0.0f;
        isInExtraLifeMode = false;
        lastExtraLifeType = "";
        lastOrnateLevel = -1;

        // Reset kong wins and tile system
        kongWins = 0;
        totalWins = 0;
        currentTileCount = 4;
        waterHealingAmount = 0;
        windLengthReduction = 0;

        // Reset tile system completely
        mahjongTiles.availableTypes.clear();
        mahjongTiles.availableTypes.push_back(PLAIN_TILES);
        mahjongTiles.goldTiles.clear();
        mahjongTiles.tileCounts.clear();

        // Reinitialize tile counts
        for (int i = 1; i <= 9; i++) {
            mahjongTiles.tileCounts[make_pair(i, PLAIN_TILES)] = 0;
            mahjongTiles.tileCounts[make_pair(i, HAT_TILES)] = 0;
            mahjongTiles.tileCounts[make_pair(i, DOT_TILES)] = 0;
        }

        mahjongTiles.SetTileCount(4, 0);

        // CRITICAL FIX: Generate fresh next tile after complete reset
        mahjongTiles.GenerateNextTile(0.0f);
        cout << "Fresh next tile generated after reset: " << mahjongTiles.nextTile.ToString() << endl;

        score = 0;
    }

    void LoadHighScore()
    {
#ifdef PLATFORM_WEB
        // For web version, use localStorage via Emscripten
        EM_ASM({
            var highScore = localStorage.getItem('mahjong_loong_highscore');
            if (highScore) {
                setValue($0, parseInt(highScore), 'i32');
            } else {
                setValue($0, 0, 'i32');
            }
        }, &highScore);
#else
        // Desktop version uses file I/O
        ifstream file("highscore.txt");
        if (file.is_open())
        {
            file >> highScore;
            file.close();
        }
        else
        {
            highScore = 0;
        }
#endif
    }

    void SaveHighScore()
    {
#ifdef PLATFORM_WEB
        // For web version, save to localStorage
        EM_ASM({
            localStorage.setItem('mahjong_loong_highscore', $0);
        }, highScore);
#else
        // Desktop version uses file I/O
        ofstream file("highscore.txt");
        if (file.is_open())
        {
            file << highScore;
            file.close();
        }
#endif
    }

    void SaveProgressData() {
#ifdef PLATFORM_WEB
        // For web version, save to localStorage as JSON-like string
        string progressData = "";

        // Save LOONG-specific high scores
        for (auto& entry : loongHighScores) {
            progressData += "HIGH " + to_string(entry.first.first) + " " +
                           to_string(entry.first.second) + " " + to_string(entry.second) + "\n";
        }

        // Save unlocked difficulties
        for (auto& entry : difficultyUnlocked) {
            if (entry.second) { // Only save unlocked ones
                progressData += "UNLOCK " + to_string(entry.first.first) + " " +
                               to_string(entry.first.second) + "\n";
            }
        }

        // Save latent upgrade total scores
        for (auto& entry : loongTotalScores) {
            if (entry.second > 0) { // Only save if there's progress
                progressData += "TOTAL " + to_string(entry.first) + " " + to_string(entry.second) + "\n";
                cout << "Saving total score: LOONG " << entry.first << " = " << entry.second << endl;
            }
        }

        // Save latent upgrade levels
        for (auto& entry : loongUpgradeLevel) {
            if (entry.second > 0) { // Only save if there's progress
                progressData += "LEVEL " + to_string(entry.first) + " " + to_string(entry.second) + "\n";
                cout << "Saving upgrade level: LOONG " << entry.first << " = Level " << entry.second << endl;
            }
        }

        // Save to localStorage
        EM_ASM({
            localStorage.setItem('mahjong_loong_progress', UTF8ToString($0));
        }, progressData.c_str());

        cout << "Progress data saved to browser storage!" << endl;
#else
        // Desktop version uses file I/O
        ofstream file("progress.txt");
        if (file.is_open()) {
            // Save LOONG-specific high scores
            for (auto& entry : loongHighScores) {
                file << "HIGH " << entry.first.first << " " << entry.first.second << " " << entry.second << endl;
            }

            // Save unlocked difficulties
            for (auto& entry : difficultyUnlocked) {
                if (entry.second) { // Only save unlocked ones
                    file << "UNLOCK " << entry.first.first << " " << entry.first.second << endl;
                }
            }

            // Save latent upgrade total scores
            for (auto& entry : loongTotalScores) {
                if (entry.second > 0) { // Only save if there's progress
                    file << "TOTAL " << entry.first << " " << entry.second << endl;
                }
            }

            // Save latent upgrade levels
            for (auto& entry : loongUpgradeLevel) {
                if (entry.second > 0) { // Only save if there's progress
                    file << "LEVEL " << entry.first << " " << entry.second << endl;
                }
            }

            file.close();
            cout << "Progress data saved successfully!" << endl;
        }
#endif
    }

    void LoadProgressData() {
#ifdef PLATFORM_WEB
        // For web version, try to load from localStorage
        char* progressData = (char*)EM_ASM_PTR({
            var data = localStorage.getItem('mahjong_loong_progress');
            if (data) {
                var lengthBytes = lengthBytesUTF8(data) + 1;
                var stringOnWasmHeap = _malloc(lengthBytes);
                stringToUTF8(data, stringOnWasmHeap, lengthBytes);
                return stringOnWasmHeap;
            }
            return 0;
        });

        if (progressData) {
            // Parse the progress data
            string data(progressData);
            free(progressData);

            istringstream iss(data);
            string line;

            while (getline(iss, line)) {
                istringstream lineStream(line);
                string type;
                lineStream >> type;

                if (type == "HIGH") {
                    int loongType, difficulty, score;
                    lineStream >> loongType >> difficulty >> score;
                    pair<LoongType, DifficultyLevel> key = {(LoongType)loongType, (DifficultyLevel)difficulty};
                    loongHighScores[key] = score;
                } else if (type == "UNLOCK") {
                    int loongType, difficulty;
                    lineStream >> loongType >> difficulty;
                    pair<LoongType, DifficultyLevel> key = {(LoongType)loongType, (DifficultyLevel)difficulty};
                    difficultyUnlocked[key] = true;
                } else if (type == "TOTAL") {
                    int loongType, totalScore;
                    lineStream >> loongType >> totalScore;
                    loongTotalScores[(LoongType)loongType] = totalScore;
                    cout << "Loaded total score: LOONG " << loongType << " = " << totalScore << endl;
                } else if (type == "LEVEL") {
                    int loongType, level;
                    lineStream >> loongType >> level;
                    loongUpgradeLevel[(LoongType)loongType] = level;
                    cout << "Loaded upgrade level: LOONG " << loongType << " = Level " << level << endl;
                }
            }

            cout << "Progress loaded from browser storage!" << endl;
        } else {
            // First time playing - start fresh
            cout << "First time playing - starting fresh!" << endl;

            // Reset all high scores to 0
            for (auto& entry : loongHighScores) {
                entry.second = 0;
            }

            // Reset all difficulty unlocks except first dragon's first difficulty
            for (auto& entry : difficultyUnlocked) {
                if (entry.first.first == BASIC_LOONG && entry.first.second == FOUNDATION_BUILDING) {
                    entry.second = true; // Keep first dragon's first difficulty unlocked
                } else {
                    entry.second = false; // Lock everything else
                }
            }
        }
#else
        // Desktop version loads from progress.txt file
        ifstream file("progress.txt");
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                istringstream lineStream(line);
                string type;
                lineStream >> type;

                if (type == "HIGH") {
                    int loongType, difficulty, score;
                    lineStream >> loongType >> difficulty >> score;
                    pair<LoongType, DifficultyLevel> key = {(LoongType)loongType, (DifficultyLevel)difficulty};
                    loongHighScores[key] = score;
                } else if (type == "UNLOCK") {
                    int loongType, difficulty;
                    lineStream >> loongType >> difficulty;
                    pair<LoongType, DifficultyLevel> key = {(LoongType)loongType, (DifficultyLevel)difficulty};
                    difficultyUnlocked[key] = true;
                } else if (type == "TOTAL") {
                    int loongType, totalScore;
                    lineStream >> loongType >> totalScore;
                    loongTotalScores[(LoongType)loongType] = totalScore;
                } else if (type == "LEVEL") {
                    int loongType, level;
                    lineStream >> loongType >> level;
                    loongUpgradeLevel[(LoongType)loongType] = level;
                }
            }
            file.close();
            cout << "Progress loaded from progress.txt!" << endl;
        } else {
            // First time playing - start fresh
            cout << "First time playing - starting fresh!" << endl;

            // Reset all high scores to 0
            for (auto& entry : loongHighScores) {
                entry.second = 0;
            }

            // Reset all difficulty unlocks except first dragon's first difficulty
            for (auto& entry : difficultyUnlocked) {
                if (entry.first.first == BASIC_LOONG && entry.first.second == FOUNDATION_BUILDING) {
                    entry.second = true; // Keep first dragon's first difficulty unlocked
                } else {
                    entry.second = false; // Lock everything else
                }
            }

            cout << "Fresh game state initialized - only Fa Cai Foundation Building unlocked!" << endl;
        }
#endif
    }

    void CheckCollisionWithTail()
    {
        deque<Vector2> headlessBody = snake.body;
        headlessBody.pop_front();
        if (ElementInDeque(snake.body[0], headlessBody))
        {
            // Move head back to previous valid position immediately by shrinking
            if (snake.body.size() > 1) {
                snake.body.pop_front();
            }
            snake.direction = {0, 0}; // Hard stop: prevent tunneling through body

            // Check for Granite Will immunity (Earth/White LOONG)
            if (isGraniteWillActive && wallImmunities > 0) {
                wallImmunities--;
                cout << ">>> GRANITE WILL IMMUNITY: Stopped snake! Immunities remaining: " << wallImmunities << endl;

                // Stop the snake instead of death - move back one step
                if (snake.body.size() > 1) {
                    snake.body[0] = snake.body[1]; // Move head back to previous position
                }

                // Play immunity sound (optimized)
                PlaySoundSafe(eatSound, 1.5f, GetTime(), lastSoundTime, soundCooldown);

                return; // Don't die, just stop
            }

            // Check for teleport charges (Shadow LOONG Dimensional Rift)
            if (teleportCharges > 0) {
                teleportCharges--;
                cout << ">>> DIMENSIONAL RIFT ACTIVATED! Teleporting to safety! Charges remaining: " << teleportCharges << endl;

                // Teleport snake to random safe position
                Vector2 safePos;
                do {
                    safePos.x = GetRandomValue(2, cellCount - 3);
                    safePos.y = GetRandomValue(2, cellCount - 3);
                } while (ElementInDeque(safePos, snake.body));

                snake.body[0] = safePos;

                // Play special teleport sound (optimized)
                PlaySoundSafe(eatSound, 0.6f, GetTime(), lastSoundTime, soundCooldown);

                return; // Don't die
            }

            HandleDeath();
        }
    }

    // Latent upgrade display
    void DrawLatentUpgradeInfo() {
        int upgradeX = canvasWidth - 450;
        int upgradeY = 300;

        DrawText("Latent Upgrades:", upgradeX, upgradeY, 24, GOLD);
        DrawText("(Total score across all runs)", upgradeX, upgradeY + 25, 16, GRAY);

        // Debug: Show current selected level
        char debugText[100];
        sprintf(debugText, "Currently Selected: Level %d", selectedLatentLevel);
        DrawText(debugText, upgradeX, upgradeY + 45, 16, YELLOW);

        // Show upgrade level descriptions with clickable selection
        vector<string> levelDescriptions = {
            "Lv1: 1 own upgrade tile",
            "Lv2: 1 own + 1 other tile",
            "Lv3: 2 own + 1 other tile",
            "Lv4: 3 own + 1 other tile",
            "Lv5: 3 own + 2 other tiles"
        };

        DrawText("Click to select Latent Cultivation level:", upgradeX, upgradeY + 70, 14, YELLOW);

        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < 5; i++) {
            int levelY = upgradeY + 95 + i * 22;
            Rectangle levelRect = {(float)upgradeX, (float)(levelY - 2), 350, 20};

            bool isUnlocked = (loongUpgradeLevel[selectedLoongType] >= i + 1);
            bool isHovered = CheckCollisionPointRec(mousePos, levelRect);
            bool isSelected = (selectedLatentLevel == i + 1);

            Color descColor = LIGHTGRAY;
            Color bgColor = BLACK;

            if (isSelected) {
                // Currently selected level - bright highlight
                bgColor = GOLD;
                descColor = BLACK;
            } else if (isUnlocked) {
                descColor = GREEN;
                if (isHovered) {
                    bgColor = DARKGREEN;
                    descColor = WHITE;
                }
            } else {
                descColor = DARKGRAY;
                if (isHovered) {
                    bgColor = MAROON;
                }
            }

            // Draw background for hover effect
            if (isHovered) {
                DrawRectangleRec(levelRect, bgColor);
                DrawRectangleLinesEx(levelRect, 1, WHITE);
            }

            // Draw level description
            DrawText(levelDescriptions[i].c_str(), upgradeX + 5, levelY, 14, descColor);

            // Draw unlock status
            if (isUnlocked) {
                DrawText("✓", upgradeX + 320, levelY, 14, GREEN);
            } else {
                DrawText("✗", upgradeX + 320, levelY, 14, RED);
            }

            // Handle clicks to select latent cultivation level
            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                cout << "Mouse clicked on level " << (i + 1) << ", isUnlocked: " << isUnlocked << endl;
                if (isUnlocked) {
                    // Set the selected latent cultivation level
                    selectedLatentLevel = i + 1;
                    cout << "✅ SELECTED Latent Cultivation Level " << (i + 1) << " for tile generation! selectedLatentLevel = " << selectedLatentLevel << endl;
                } else {
                    cout << "❌ Latent Cultivation Level " << (i + 1) << " not unlocked yet!" << endl;
                }
            }
        }

        // Show current LOONG's progress prominently
        LoongType currentLoong = selectedLoongType;
        int currentLevel = loongUpgradeLevel[currentLoong];
        int currentScore = loongTotalScores[currentLoong];

        DrawText("Current LOONG:", upgradeX, upgradeY + 180, 20, YELLOW);

        char currentText[150];
        if (currentLevel >= 5) {
            sprintf(currentText, "%s: MAX LEVEL (%d total)",
                   availableLOONGs[selectedLoongIndex].name.c_str(), currentScore);
        } else {
            int nextThreshold = latentUpgradeThresholds[currentLevel];
            sprintf(currentText, "%s: Lv%d - %d/%d to Lv%d",
                   availableLOONGs[selectedLoongIndex].name.c_str(),
                   currentLevel, currentScore, nextThreshold, currentLevel + 1);
        }

        DrawText(currentText, upgradeX, upgradeY + 205, 16, WHITE);

        // Progress bar for current LOONG
        if (currentLevel < 5) {
            int nextThreshold = latentUpgradeThresholds[currentLevel];
            int prevThreshold = (currentLevel > 0) ? latentUpgradeThresholds[currentLevel - 1] : 0;

            float progress = (float)(currentScore - prevThreshold) / (float)(nextThreshold - prevThreshold);
            progress = Clamp(progress, 0.0f, 1.0f);

            Rectangle progressBg = {(float)upgradeX, (float)(upgradeY + 230), 300, 20};
            Rectangle progressBar = {progressBg.x, progressBg.y, progressBg.width * progress, progressBg.height};

            DrawRectangleRec(progressBg, DARKGRAY);
            DrawRectangleRec(progressBar, BLUE);
            DrawRectangleLinesEx(progressBg, 2, WHITE);

            // Progress text
            char progressText[50];
            sprintf(progressText, "%.0f%%", progress * 100);
            int textWidth = MeasureText(progressText, 16);
            DrawText(progressText, upgradeX + 150 - textWidth/2, upgradeY + 232, 16, WHITE);
        }
    }

    // Audio control functions
    void ToggleMute() {
        isMuted = !isMuted;
        if (isMuted) {
            SetMasterVolume(0.0f);
            cout << "Audio muted" << endl;
        } else {
            SetMasterVolume(masterVolume);
            cout << "Audio unmuted" << endl;
        }
    }

    void SetVolume(float volume) {
        masterVolume = Clamp(volume, 0.0f, 1.0f);
        if (!isMuted) {
            SetMasterVolume(masterVolume);
            // Update individual music volumes
            if (titleScreenMusic.stream.buffer != NULL) {
                SetMusicVolume(titleScreenMusic, masterVolume);
            }
            if (loongSelectMusic.stream.buffer != NULL) {
                SetMusicVolume(loongSelectMusic, masterVolume);
            }
            if (backgroundMusic.stream.buffer != NULL) {
                SetMusicVolume(backgroundMusic, masterVolume);
            }
        }
    }

    // Latent upgrade system
    void InitializeLatentUpgrades() {
        for (int i = 0; i < 9; i++) {
            loongTotalScores[(LoongType)i] = 0;
            loongUpgradeLevel[(LoongType)i] = 0;
        }
        cout << "Latent upgrade system initialized" << endl;
    }

    void UpdateLoongTotalScore(int scoreGained) {
        loongTotalScores[selectedLoongType] += scoreGained;

        // Debug output
        cout << "Updated " << availableLOONGs[selectedLoongIndex].name << " total score: " << loongTotalScores[selectedLoongType] << " (+" << scoreGained << ")" << endl;

        // Check for upgrade level increases
        int oldLevel = loongUpgradeLevel[selectedLoongType];
        int newLevel = 0;

        for (int i = 0; i < (int)latentUpgradeThresholds.size(); i++) {
            if (loongTotalScores[selectedLoongType] >= latentUpgradeThresholds[i]) {
                newLevel = i + 1;
            }
        }

        if (newLevel > oldLevel) {
            loongUpgradeLevel[selectedLoongType] = newLevel;
            cout << "🎉 " << availableLOONGs[selectedLoongIndex].name << " reached upgrade level " << newLevel << "! Total score: " << loongTotalScores[selectedLoongType] << " 🎉" << endl;
            // Save immediately when level increases
            SaveProgressData();
        }
    }

    bool HasLatentUpgrade() {
        return loongUpgradeLevel[selectedLoongType] > 0;
    }

    void SpawnLatentCultivationUpgrades() {
        if (selectedLatentLevel <= 0) return;

        cout << "🌟 Queuing Latent Cultivation Level " << selectedLatentLevel << " LOONG evolution tiles!" << endl;

        // Clear any existing queue
        pendingLatentUpgrades.clear();

        // Count how many upgrade tiles to spawn based on level
        int ownUpgrades = 0;
        int otherUpgrades = 0;

        if (selectedLatentLevel >= 1) ownUpgrades = 1;
        if (selectedLatentLevel >= 2) otherUpgrades = 1;
        if (selectedLatentLevel >= 3) ownUpgrades = 2;
        if (selectedLatentLevel >= 4) ownUpgrades = 3;
        if (selectedLatentLevel >= 5) otherUpgrades = 2;

        // Add own LOONG evolution tiles to queue
        for (int i = 0; i < ownUpgrades; i++) {
            pendingLatentUpgrades.push_back(selectedLoongType);
        }

        // Add other LOONG evolution tiles to queue
        for (int i = 0; i < otherUpgrades; i++) {
            // Pick a random other LOONG type
            LoongType otherLoong;
            do {
                otherLoong = (LoongType)(rand() % 9);
            } while (otherLoong == selectedLoongType);

            pendingLatentUpgrades.push_back(otherLoong);
        }

        // Start the spawning timer
        latentUpgradeSpawnTimer = 0.5f; // Start spawning after 0.5 seconds

        cout << "✅ Queued " << pendingLatentUpgrades.size() << " LOONG evolution tiles for spawning!" << endl;
    }
};

int main()
{
    cout << "Starting the Mahjong Snake game..." << endl;
    InitWindow(canvasWidth, canvasHeight, "Mahjong Snake - Mouse + Tile Matching");
    SetTargetFPS(60);

    Game game = Game();

    while (WindowShouldClose() == false)
    {
        BeginDrawing();

        // Handle input
        game.HandleInput();

        // Always update music and countdown for responsiveness
        game.UpdateMusicAndCountdown();

        // Apply speed multiplier from power-ups
        float baseSpeed = 0.2f;
        float actualSpeed = baseSpeed / game.currentSpeedMultiplier;

        if (EventTriggered(actualSpeed)) // Speed affected by power-ups
        {
            game.UpdateGameplay(); // This now checks if choice window is open
        }

        // Game drawing
        game.Draw();

        EndDrawing();
    }
    CloseWindow();
    return 0;
}