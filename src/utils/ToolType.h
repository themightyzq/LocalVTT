#ifndef TOOLTYPE_H
#define TOOLTYPE_H

enum class ToolType {
    Pointer = 0,       // Default: Pointer/beacon tool (double-click anywhere)
    FogBrush = 1,      // Fog of War brush tools
    FogRectangle = 2   // Fog of War rectangle reveal tool
    // NOTE: Pan is NOT a tool - use middle-click drag to pan (CLAUDE.md)
};

#endif // TOOLTYPE_H