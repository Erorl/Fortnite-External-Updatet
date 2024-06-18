#include <windows.h>
// Visual Features | ESP

bool outlined = false;
bool visiblecheck = false;
bool player_distance = false;
bool skeletonesp = false;
bool cornerbox = false;
bool bottom_tracers = false;
bool Esp = false;
bool box = false;
bool fillbox = false;

// Aim Features | Aimbot

bool aimbotvisibleonly = false;
bool Aimbot = false;
bool fovcircle = false;

// Exploits

bool spinbot = false;
bool norecoil = false;

// Misc Features | Misc

bool ShowMenu = true;
bool crosshair = false;
bool fpsCounter = true;

// ESP Values, Aimbot Values | Other

bool debug_developer = false;
float box_thickness = 1;
float tracers_thickness = 1;
float skeleton_thickness = 3.5;
float BoxWidthValue = 0.15;
float ChangerFOV = 80;
float smooth = 5.0f;
float AimFOV = 200.0f;
static int VisDist = 2400;
static int aimkey;
static int hitbox;
static int hitboxpos = 0;
static int tracerpos = 0;
static int aimkeypos = 3;
static int aimbone = 1;
int faken_rot = 0;