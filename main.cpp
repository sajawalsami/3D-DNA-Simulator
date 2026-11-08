
#include "raylib.h"
#include <cmath>
#include <cstring>

const int MAX_SEQUENCE = 128;
const int SIDEBAR_WIDTH = 360;

// Find complementary DNA base
char getComplement(char base)
{
    switch (base)
    {
        case 'A': return 'T';
        case 'T': return 'A';
        case 'C': return 'G';
        case 'G': return 'C';
        default: return '?';
    }
}

// Nucleotide colors
Color getBaseColor(char base)
{
    switch (base)
    {
        case 'A': return BLUE;
        case 'T': return RED;
        case 'C': return GREEN;
        case 'G': return ORANGE;
        default: return GRAY;
    }
}

// Validate DNA characters
bool isValidBase(char base)
{
    return base == 'A' || base == 'T' ||
           base == 'C' || base == 'G';
}

// Calculate helix position
Vector3 helixPoint(float radius, float angle, float y)
{
    return {
        radius * cosf(angle),
        y,
        radius * sinf(angle)
    };
}

// Draw connection between two 3D points
void drawConnection(Vector3 start, Vector3 end,
                    float thickness, Color color)
{
    DrawCylinderEx(
        start, end,
        thickness, thickness,
        8, color
    );
}

void drawBackground(int width, int height)
{
    DrawRectangleGradientV(
        0, 0, width, height,
        Color{ 7, 13, 31, 255 },
        Color{ 12, 31, 49, 255 }
    );

    DrawCircleGradient(
        { width * 0.73f, height * 0.43f },
        width * 0.38f,
        Color{ 26, 105, 145, 42 },
        Color{ 26, 105, 145, 0 }
    );

    DrawCircleGradient(
        { width * 0.91f, height * 0.82f },
        width * 0.28f,
        Color{ 45, 70, 145, 32 },
        Color{ 45, 70, 145, 0 }
    );

    for (int x = SIDEBAR_WIDTH + 20; x < width; x += 72)
        DrawLineEx(
            { (float)x, 0.0f }, { (float)x, (float)height },
            1.0f, Color{ 112, 176, 205, 14 }
        );

    for (int y = 24; y < height; y += 72)
        DrawLineEx(
            { (float)SIDEBAR_WIDTH, (float)y }, { (float)width, (float)y },
            1.0f, Color{ 112, 176, 205, 14 }
        );

    for (int i = 0; i < 72; i++)
    {
        int x = SIDEBAR_WIDTH + 10 +
            (i * 197 + 43) % (width - SIDEBAR_WIDTH - 10);
        int y = (i * 131 + 29) % height;
        float radius = 1.0f + (i % 3) * 0.45f;
        unsigned char alpha = (unsigned char)(65 + (i % 4) * 22);

        DrawCircle(x, y, radius, Color{ 190, 225, 255, alpha });
    }
}

void drawViewerFrame(int width, int height)
{
    Rectangle frame = {
        (float)SIDEBAR_WIDTH + 18.0f,
        74.0f,
        (float)width - SIDEBAR_WIDTH - 42.0f,
        (float)height - 98.0f
    };

    // Keep the 3D scene on a flat, opaque surface so it stays distinct
    // from the decorative background when the camera rotates.
    DrawRectangleRec(frame, Color{ 2, 5, 12, 255 });

    DrawRectangleLinesEx(
        frame, 2.0f, Color{ 107, 181, 210, 190 }
    );
}

int sliderBasePair(float mouseX, float sliderStartX,
                   float sliderEndX, int basePairCount)
{
    if (basePairCount <= 1) return 1;

    float position =
        (mouseX - sliderStartX) / (sliderEndX - sliderStartX);

    if (position < 0.0f) position = 0.0f;
    if (position > 1.0f) position = 1.0f;

    return 1 + (int)(position * (basePairCount - 1) + 0.5f);
}

int main()
{
    InitWindow(1100, 750, "DNA Molecular Simulator");
    SetTargetFPS(60);

    const int viewerX = SIDEBAR_WIDTH + 20;
    const int viewerY = 88;
    const int viewerWidth = GetScreenWidth() - SIDEBAR_WIDTH - 64;
    const int viewerHeight = GetScreenHeight() - 132;
    RenderTexture2D viewerTexture =
        LoadRenderTexture(viewerWidth, viewerHeight);

    // DNA sequence
    char sequence[MAX_SEQUENCE] = "ATCGATCGATCGATCG";
    char inputBuffer[MAX_SEQUENCE] = "";

    bool inputActive = false;
    bool inputError = false;
    bool selectAll = false;
    bool separationDragging = false;
    bool geneStartDragging = false;
    bool geneEndDragging = false;
    float separationAmount = 0.0f;
    int geneStart = 1;
    int geneEnd = 4;

    Rectangle inputBox = { 20, 260, 320, 45 };
    Rectangle separationSlider = { 20, 445, 320, 48 };
    Rectangle geneStartSlider = { 104, 574, 225, 22 };
    Rectangle geneEndSlider = { 104, 605, 225, 22 };
    const float sliderStartX = 30.0f;
    const float sliderEndX = 330.0f;
    const float geneSliderStartX = 112.0f;
    const float geneSliderEndX = 320.0f;

    // Camera
    Camera3D camera = { 0 };

    Vector3 target = { 0.0f, 3.0f, 0.0f };

    float yaw = 0.7f;
    float pitch = 0.35f;
    float distance = 15.0f;

    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // DNA geometry
    const float backboneRadius = 2.0f;
    const float baseRadius = 0.85f;
    const float spacing = 0.4f;
    const float twist = 0.5f;

    while (!WindowShouldClose())
    {
        Vector2 mouse = GetMousePosition();

        // INPUT BOX ACTIVATION
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            inputActive = CheckCollisionPointRec(mouse, inputBox);
            selectAll = false;
            separationDragging =
                CheckCollisionPointRec(mouse, separationSlider);
            geneStartDragging =
                CheckCollisionPointRec(mouse, geneStartSlider);
            geneEndDragging =
                CheckCollisionPointRec(mouse, geneEndSlider);
        }

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            separationDragging = false;
            geneStartDragging = false;
            geneEndDragging = false;
        }

        if (separationDragging)
        {
            separationAmount = (mouse.x - sliderStartX) /
                (sliderEndX - sliderStartX);

            if (separationAmount < 0.0f) separationAmount = 0.0f;
            if (separationAmount > 1.0f) separationAmount = 1.0f;
        }

        if (inputActive)
        {
            // SELECT ALL
            if (IsKeyDown(KEY_LEFT_CONTROL) &&
                IsKeyPressed(KEY_A))
            {
                selectAll = true;
            }

            // CLEAR SELECTED TEXT BEFORE TYPING
            if (selectAll)
            {
                if (GetCharPressed() > 0 ||
                    IsKeyPressed(KEY_BACKSPACE))
                {
                    inputBuffer[0] = '\0';
                    selectAll = false;
                }
            }

            // PASTE FROM CLIPBOARD
            if ((IsKeyDown(KEY_LEFT_CONTROL) ||
                 IsKeyDown(KEY_RIGHT_CONTROL)) &&
                IsKeyPressed(KEY_V))
            {
                const char* clipboard = GetClipboardText();

                if (clipboard != nullptr)
                {
                    int length = selectAll ? 0 :
                        (int)strlen(inputBuffer);

                    if (selectAll)
                        inputBuffer[0] = '\0';

                    selectAll = false;

                    for (int i = 0;
                         clipboard[i] != '\0' &&
                         length < MAX_SEQUENCE - 1;
                         i++)
                    {
                        char character = clipboard[i];

                        if (character >= 'a' && character <= 'z')
                            character -= 32;

                        if (isValidBase(character))
                        {
                            inputBuffer[length++] = character;
                        }
                    }

                    inputBuffer[length] = '\0';
                    inputError = false;
                }
            }

            // TYPE CHARACTERS
            int key = GetCharPressed();

            while (key > 0)
            {
                char character = (char)key;

                if (character >= 'a' && character <= 'z')
                    character -= 32;

                if (isValidBase(character))
                {
                    int length = (int)strlen(inputBuffer);

                    if (selectAll)
                    {
                        inputBuffer[0] = '\0';
                        length = 0;
                        selectAll = false;
                    }

                    if (length < MAX_SEQUENCE - 1)
                    {
                        inputBuffer[length] = character;
                        inputBuffer[length + 1] = '\0';
                        inputError = false;
                    }
                }

                key = GetCharPressed();
            }

            // BACKSPACE
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                if (selectAll)
                {
                    inputBuffer[0] = '\0';
                    selectAll = false;
                }
                else
                {
                    int length = (int)strlen(inputBuffer);

                    if (length > 0)
                        inputBuffer[length - 1] = '\0';
                }
            }

            // GENERATE DNA
            if (IsKeyPressed(KEY_ENTER))
            {
                if (strlen(inputBuffer) > 0)
                {
                    strcpy(sequence, inputBuffer);
                    inputBuffer[0] = '\0';
                    inputError = false;
                    selectAll = false;
                }
                else
                {
                    inputError = true;
                }
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                inputActive = false;
                selectAll = false;
            }
        }
        else
        {
            // CAMERA ROTATION
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                Vector2 delta = GetMouseDelta();

                yaw -= delta.x * 0.005f;
                pitch -= delta.y * 0.005f;

                if (pitch > 1.45f) pitch = 1.45f;
                if (pitch < -1.45f) pitch = -1.45f;
            }

            // ZOOM
            float wheel = GetMouseWheelMove();

            distance -= wheel * 1.5f;

            if (distance < 5.0f) distance = 5.0f;
            if (distance > 80.0f) distance = 80.0f;
        }

        int basePairs = (int)strlen(sequence);
        if (geneStart > basePairs) geneStart = basePairs;
        if (geneEnd > basePairs) geneEnd = basePairs;

        if (geneStartDragging)
        {
            geneStart = sliderBasePair(
                mouse.x, geneSliderStartX, geneSliderEndX, basePairs
            );
            if (geneStart > geneEnd) geneEnd = geneStart;
        }

        if (geneEndDragging)
        {
            geneEnd = sliderBasePair(
                mouse.x, geneSliderStartX, geneSliderEndX, basePairs
            );
            if (geneEnd < geneStart) geneStart = geneEnd;
        }

        // CAMERA POSITION
        target.x = 0.0f;
        target.y = (basePairs - 1) * spacing / 2.0f;
        target.z = 0.0f;
        camera.target = target;

        camera.position = {
            target.x + distance * cosf(pitch) * sinf(yaw),
            target.y + distance * sinf(pitch),
            target.z + distance * cosf(pitch) * cosf(yaw)
        };

        // DRAWING
        BeginDrawing();

        drawBackground(GetScreenWidth(), GetScreenHeight());
        drawViewerFrame(GetScreenWidth(), GetScreenHeight());

        BeginTextureMode(viewerTexture);
        ClearBackground(Color{ 2, 5, 12, 255 });
        BeginMode3D(camera);

        Vector3 previous1 = { 0, 0, 0 };
        Vector3 previous2 = { 0, 0, 0 };

        for (int i = 0; i < basePairs; i++)
        {
            float angle = i * twist;
            float y = i * spacing;
            float splitOffset = separationAmount * 1.8f;

            Vector3 backbone1 =
                helixPoint(backboneRadius + splitOffset, angle, y);

            Vector3 backbone2 =
                helixPoint(backboneRadius + splitOffset, angle + PI, y);

            Vector3 base1 =
                helixPoint(baseRadius + splitOffset, angle, y);

            Vector3 base2 =
                helixPoint(baseRadius + splitOffset, angle + PI, y);

            bool isGeneBasePair = i >= geneStart - 1 && i < geneEnd;
            Color backboneColor = isGeneBasePair
                ? Color{ 49, 205, 194, 255 }
                : LIGHTGRAY;
            Color rungColor = isGeneBasePair
                ? Color{ 49, 205, 194, 255 }
                : WHITE;

            if (i > 0)
            {
                drawConnection(
                    previous1, backbone1, 0.11f, backboneColor
                );

                drawConnection(
                    previous2, backbone2, 0.11f, backboneColor
                );
            }

            DrawSphere(backbone1, 0.16f, backboneColor);
            DrawSphere(backbone2, 0.16f, backboneColor);

            drawConnection(
                backbone1, base1, 0.07f, GOLD
            );

            drawConnection(
                backbone2, base2, 0.07f, GOLD
            );

            char first = sequence[i];
            char second = getComplement(first);

            if (isGeneBasePair)
            {
                DrawSphere(base1, 0.27f, Color{ 49, 205, 194, 255 });
                DrawSphere(base2, 0.27f, Color{ 49, 205, 194, 255 });
            }

            DrawSphere(base1, 0.20f, getBaseColor(first));
            DrawSphere(base2, 0.20f, getBaseColor(second));

            drawConnection(
                base1, base2, 0.055f, rungColor
            );

            previous1 = backbone1;
            previous2 = backbone2;
        }

        EndMode3D();
        EndTextureMode();
        DrawTextureRec(
            viewerTexture.texture,
            { 0.0f, 0.0f, (float)viewerWidth, (float)-viewerHeight },
            { (float)viewerX, (float)viewerY },
            WHITE
        );

        // USER INTERFACE
        DrawRectangleGradientH(
            0, 0, SIDEBAR_WIDTH, GetScreenHeight(),
            Color{ 5, 11, 27, 238 },
            Color{ 8, 18, 35, 255 }
        );
        DrawLineEx(
            { (float)SIDEBAR_WIDTH, 0.0f },
            { (float)SIDEBAR_WIDTH, (float)GetScreenHeight() },
            1.0f, Color{ 98, 177, 210, 115 }
        );

        DrawRectangle(14, 52, SIDEBAR_WIDTH - 28, 137,
                      Color{ 19, 34, 54, 180 });
        DrawRectangleLinesEx(
            { 14, 52, SIDEBAR_WIDTH - 28.0f, 137 },
            1.0f, Color{ 91, 150, 182, 65 }
        );

        DrawRectangle(14, 222, SIDEBAR_WIDTH - 28, 171,
                      Color{ 19, 34, 54, 180 });
        DrawRectangleLinesEx(
            { 14, 222, SIDEBAR_WIDTH - 28.0f, 171 },
            1.0f, Color{ 91, 150, 182, 65 }
        );

        DrawRectangle(14, 410, SIDEBAR_WIDTH - 28, 128,
                      Color{ 19, 34, 54, 180 });
        DrawRectangleLinesEx(
            { 14, 410, SIDEBAR_WIDTH - 28.0f, 128 },
            1.0f, Color{ 91, 150, 182, 65 }
        );

        DrawRectangle(14, 550, SIDEBAR_WIDTH - 28, 94,
                      Color{ 19, 34, 54, 180 });
        DrawRectangleLinesEx(
            { 14, 550, SIDEBAR_WIDTH - 28.0f, 94 },
            1.0f, Color{ 91, 150, 182, 65 }
        );

        DrawText(
            "DNA SIMULATOR",
            20, 20, 25, RAYWHITE
        );
        DrawText("MOLECULAR LAB  /  3D VIEW", SIDEBAR_WIDTH + 38, 29, 16,
                 Color{ 157, 195, 216, 255 });
        DrawText("Rotate with right-drag  |  Zoom with wheel",
                 SIDEBAR_WIDTH + 38, 51, 15,
                 Color{ 124, 160, 181, 255 });
        DrawText("LIVE STRUCTURE", SIDEBAR_WIDTH + 38,
                 GetScreenHeight() - 49, 14,
                 Color{ 157, 195, 216, 255 });
        DrawText(TextFormat("%d BASE PAIRS", basePairs),
                 SIDEBAR_WIDTH + 160, GetScreenHeight() - 49, 14,
                 Color{ 124, 160, 181, 255 });

        DrawText("A - Adenine", 20, 65, 18, BLUE);
        DrawText("T - Thymine", 20, 90, 18, RED);
        DrawText("C - Cytosine", 20, 115, 18, GREEN);
        DrawText("G - Guanine", 20, 140, 18, ORANGE);
        DrawText("Backbone - Silver", 20, 165, 18, LIGHTGRAY);

        DrawText(
            TextFormat("Base pairs: %d", basePairs),
            20, 205, 20, RAYWHITE
        );

        DrawText(
            "Enter DNA sequence (A, T, C, G):",
            20, 235, 18, RAYWHITE
        );

        DrawRectangleRec(inputBox, RAYWHITE);

        DrawRectangleLinesEx(
            inputBox, 2,
            inputActive ? BLUE : GRAY
        );

        DrawText(inputBuffer, 30, 272, 20, BLACK);

        if (inputError)
        {
            DrawText(
                "Please enter a valid sequence.",
                20, 315, 18, RED
            );
        }
        else
        {
            DrawText(
                "Click box | CTRL+V Paste | CTRL+A Select all",
                20, 315, 16, RAYWHITE
            );

            DrawText(
                "BACKSPACE Delete | ENTER Generate | ESC Exit",
                20, 338, 16, RAYWHITE
            );

        }

        DrawText("STRAND SEPARATION", 24, 426, 16,
                 Color{ 205, 222, 237, 255 });
        DrawText(TextFormat("%d%%", (int)(separationAmount * 100.0f)),
                 294, 426, 16, Color{ 79, 205, 220, 255 });

        int sliderFillWidth =
            (int)((sliderEndX - sliderStartX) * separationAmount);
        int sliderHandleX =
            (int)(sliderStartX +
                  (sliderEndX - sliderStartX) * separationAmount);
        DrawRectangle((int)sliderStartX, 467,
                      (int)(sliderEndX - sliderStartX), 6,
                      Color{ 59, 77, 97, 255 });
        DrawRectangle((int)sliderStartX, 467, sliderFillWidth, 6,
                      Color{ 49, 184, 202, 255 });
        DrawCircle(sliderHandleX, 470, 10, RAYWHITE);
        DrawCircleLines(sliderHandleX, 470, 10,
                        Color{ 49, 184, 202, 255 });
        DrawText("PAIRED", 24, 488, 13,
                 Color{ 124, 160, 181, 255 });
        DrawText("SPLIT", 294, 488, 13,
                 Color{ 124, 160, 181, 255 });
        DrawText("Drag to spread both strands apart",
                 24, 511, 14, Color{ 157, 195, 216, 255 });

        DrawText("GENE / BASE-PAIR RANGE", 24, 558, 15,
                 Color{ 205, 222, 237, 255 });
        DrawText("START", 24, 579, 12,
                 Color{ 124, 160, 181, 255 });
        DrawText(TextFormat("%03d", geneStart), 67, 578, 14,
                 Color{ 79, 205, 220, 255 });
        DrawRectangle(114, 582, 206, 4,
                      Color{ 59, 77, 97, 255 });
        DrawRectangle(114, 582,
                      (int)((geneStart - 1) * 206.0f /
                            (basePairs > 1 ? basePairs - 1 : 1)),
                      4, Color{ 49, 184, 202, 255 });
        DrawCircle(
            (int)(geneSliderStartX +
                  (geneStart - 1) * (geneSliderEndX - geneSliderStartX) /
                  (basePairs > 1 ? basePairs - 1 : 1)),
            584, 7, RAYWHITE
        );

        DrawText("END", 24, 610, 12,
                 Color{ 124, 160, 181, 255 });
        DrawText(TextFormat("%03d", geneEnd), 67, 609, 14,
                 Color{ 79, 205, 220, 255 });
        DrawRectangle(114, 613, 206, 4,
                      Color{ 59, 77, 97, 255 });
        DrawRectangle(114, 613,
                      (int)((geneEnd - 1) * 206.0f /
                            (basePairs > 1 ? basePairs - 1 : 1)),
                      4, Color{ 49, 184, 202, 255 });
        DrawCircle(
            (int)(geneSliderStartX +
                  (geneEnd - 1) * (geneSliderEndX - geneSliderStartX) /
                  (basePairs > 1 ? basePairs - 1 : 1)),
            615, 7, RAYWHITE
        );

        DrawText(
            "RIGHT MOUSE + DRAG: Rotate",
            20, 650, 18, RAYWHITE
        );

        DrawText(
            "MOUSE WHEEL: Zoom",
            20, 675, 18, RAYWHITE
        );

        DrawFPS(1000, 20);

        EndDrawing();
    }

    UnloadRenderTexture(viewerTexture);
    CloseWindow();

    return 0;
}
