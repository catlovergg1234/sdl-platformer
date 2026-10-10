#define SDL_MAIN_HANDLED
#define NOT_FOUND std::string::npos
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
Uint32 ROOM_CHANGED;

std::vector<std::string> split(const std::string& text, char seperator) {
    std::vector<std::string> result;
    std::stringstream ss(text);
    std::string part;
    while (std::getline(ss, part, seperator)) {
        result.push_back(part);
    }
    return result;
}

std::string join(
    const std::vector<int>& parts,
    const std::string separator
) {
    std::string result;

    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) {
            result += separator;
        }

        result += std::to_string(parts[i]);
    }

    return result;
}

void set_center(SDL_Point center, SDL_Rect *rect) {
    rect->x = center.x - (rect->w/2);
    rect->y = center.y - (rect->h/2);
};

class spritesheet {
    public:
    int letterwidth;
    int letterheight;
    std::string order;
    SDL_Surface* img;
    SDL_Color color;
    SDL_Color bg_color;
    float size;
    int index_container;
    spritesheet(int letterwidth, int letterheight, std::string order, const char* path, SDL_Color color, SDL_Color bg_color, float size) {
        this->letterwidth = letterwidth;
        this->letterheight = letterheight;
        this->order = order;
        this->img = IMG_Load(path);
        this->color = color;
        this->bg_color = bg_color;
        this->size = size;
    }
    ~spritesheet() {
        SDL_FreeSurface(this->img);
    }
    SDL_Surface* render(std::string text, SDL_Color* colorptr=nullptr, SDL_Color* bg_colorptr=nullptr) {
        SDL_Color color;
        SDL_Color bg_color;
        if (colorptr == nullptr) {
            color = this->color;
        } else {
            color = *colorptr;
        }
        if (bg_colorptr == nullptr) {
            bg_color = this->bg_color;
        } else {
            bg_color = *bg_colorptr;
        }
        if (text == "") {
            throw std::invalid_argument("text argument cannot be empty");
        }
        auto lines = split(text, '\n');
        std::vector<int> linelengths;
        for (const auto& line : lines) {
            linelengths.push_back(line.size());
        }
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, *std::max_element(linelengths.begin(), linelengths.end())*(this->letterwidth), (static_cast<int>(lines.size()))*(this->letterheight), 32, SDL_PIXELFORMAT_RGBA32);
        auto letterlines = split(this->order, '\n');
        int col = 0;
        int ln = 0;
        std::string line;
        size_t find;
        SDL_Color unmapped = this->color;
        SDL_Color unmapped_bg = this->bg_color;
        Uint32 mapped_color = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
        Uint32 mapped_bgcolor = SDL_MapRGBA(surface->format, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        Uint32 original_color = SDL_MapRGBA(surface->format, unmapped.r, unmapped.g, unmapped.b, unmapped.a);
        Uint32 original_bgcolor = SDL_MapRGBA(surface->format, unmapped_bg.r, unmapped_bg.g, unmapped_bg.b, unmapped_bg.a);
        SDL_Rect srcrect;
        SDL_Rect dstrect;
        for (const char letter : text) {
            if (letter == '\n') {
                ln++;
                col=0;
                continue;
            }
            for (int index = 0; index < letterlines.size(); index++) {
                line = letterlines[index];
                find = line.find(letter);
                index_container = index;
                if (find != NOT_FOUND) break;
            }
            if (find == NOT_FOUND) {
                SDL_Rect rect = {col*(this->letterwidth), ln*(this->letterheight), this->letterwidth, this->letterheight};
                SDL_FillRect(surface, &rect, mapped_color);
                col++;
                continue;
            }
            srcrect = {this->letterwidth * static_cast<int>(find), this->letterheight * index_container, this->letterwidth, this->letterheight};
            dstrect = {col*(this->letterwidth), ln*(this->letterheight), this->letterwidth, this->letterheight};
            SDL_BlitSurface(this->img, &srcrect, surface, &dstrect);
            col++;
        }
        Uint8* pixels = (Uint8*)surface->pixels;
        int pitch = surface->pitch;
        SDL_PixelFormat* format = surface->format;
        int Bpp = format->BytesPerPixel;
        Uint8* row;
        Uint8* pixel;
        Uint8 r,g,b,a;
        for (int y=0; y < surface->h; y++) {
            row = pixels + y*pitch;
            for (int x=0; x < surface->w; x++) {
                pixel = row + x*Bpp;
                Uint32 color = *(Uint32*)pixel;
                if (color == original_color) {
                    *(Uint32*) pixel = mapped_color;
                } else if (color == original_bgcolor) {
                    *(Uint32*) pixel = mapped_bgcolor;
                }
            }
        }
        if (this->size != 1.0f) {
            SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
                0,
                surface->w*this->size,
                surface->h*this->size,
                32,
                SDL_PIXELFORMAT_RGBA32
            );
            SDL_BlitScaled(
                surface,
                nullptr,
                scaled,
                nullptr
            );
            SDL_FreeSurface(surface);
            surface = scaled;
        }
        return surface;
    }
};


void draw_map(SDL_Renderer* ren, SDL_Texture* level_te) {
    SDL_Rect level = {0, 0, 600, 600};
    SDL_RenderCopy(ren, level_te, NULL, &level);
}

bool has_col(
    const std::vector<std::vector<Uint8>>& maskA,
    const SDL_Rect& rectA,
    const std::vector<std::vector<Uint8>>& maskB,
    const SDL_Rect& rectB
) {
    int left   = std::max(rectA.x, rectB.x);
    int right  = std::min(rectA.x + rectA.w, rectB.x + rectB.w);
    int top    = std::max(rectA.y, rectB.y);
    int bottom = std::min(rectA.y + rectA.h, rectB.y + rectB.h);
    
    if (right <= left || bottom <= top) return false;

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            int ax = x - rectA.x;
            int ay = y - rectA.y;
            int bx = x - rectB.x;
            int by = y - rectB.y;

            if (maskA[ay][ax] && maskB[by][bx])
                return true;
        }
    }
    return false;
}

char hasinte(SDL_Rect *player, std::vector<std::vector<Uint8>> player_mask, std::vector<std::vector<Uint8>> *level_mask, std::vector<int> *room, std::vector<std::vector<Uint8>> *spike_mask) {
    if (player->x < 0 || player->x > 600 || player->y < 0 || player->y > 600) {
        SDL_ClearError();
        SDL_Log("BOUNDARY CROSSED: x=%d y=%d ROOM_CHANGED=%u",player->x, player->y, ROOM_CHANGED);
        SDL_Event event{};
        event.type = ROOM_CHANGED;
        int result = SDL_PushEvent(&event);
        SDL_Log("PUSH RESULT = %d, error = %s",
            result, SDL_GetError());
    }
    if (player->x < 0) {
        (*room)[0] --;
        player->x = 560;
        return 'r';
    }
    if (player->x > 600) {
        (*room)[0] ++;
        player->x = 40;
        return 'r';
    }
    if (player->y < 0) {
        (*room)[1] ++;
        player->y = 520;
        return 'r';
    }
    if (player->y > 600) {
        (*room)[1] --;
        player->y = 80;
        return 'r';
    }
    if (has_col(player_mask, *player, *spike_mask, {0,0,600,600})) return 's';
    if (has_col(player_mask, *player, *level_mask, {0, 0, 600, 600})) return 'w';
    return 'n';
}

char move_y(int move, SDL_Rect* player, int* vel_y, int* rj, std::vector<std::vector<Uint8>> pm, std::vector<std::vector<Uint8>> *lm, std::vector<int> *room, std::vector<std::vector<Uint8>> *sm) {
    if (!move) {
        return 'n';
    }
    int dir = (move > 0) ? -1 : 1;
    for (int i = 0; i < abs(move); i++) {
        player->y -= dir;
        char inte = hasinte(player, pm, lm, room, sm);
        if (inte == 'w') {
            if (move>0) {
                *rj = 1;
            }
            *vel_y = 0;
            while (true) {
                player->y += dir;
                if (hasinte(player, pm, lm, room, sm) != 'w') {
                    return hasinte(player, pm, lm, room, sm);
                }
            }
        }
        if (inte == 's') return 's';
        if (inte == 'r') {
            player->y += dir;
            return 'r';
        }
        if (inte == 'n' && move>0) {
            *rj = 0;
        }
    }
    return 'n';
}

char move_x(int move, SDL_Rect* player, std::vector<std::vector<Uint8>> pm, std::vector<std::vector<Uint8>> *lm, std::vector<int> *room, std::vector<std::vector<Uint8>> *sm) {
    if (!move) {
        return 'n';
    }
    int dir = (move > 0) ? -1 : 1;
    for (int i = 0; i < abs(move); i++) {
        player->x -= dir;
        char inte = hasinte(player, pm, lm, room, sm);
        if (inte == 'w') {
            player->x += dir;
        } if (inte != 'n' && inte!= 'w') {
            return inte;
        }
        if (inte == 'r') {
            player->x += dir;
            return 'r';
        }
    }
    return 'n';
}

std::vector<std::vector<Uint8>> create_mask(SDL_Surface* surf) {
    std::vector<std::vector<Uint8>> map(surf->h, std::vector<Uint8>(surf->w, 0));
    Uint8* pixels = (Uint8*)surf->pixels;
    int pitch = surf->pitch;
    SDL_PixelFormat* format = surf->format;
    int bpp = format->BytesPerPixel;
    Uint8* row;
    for (int y = 0; y < surf->h; y++) {
        row = pixels + y * pitch;
        for (int x = 0; x < surf->w; x++) {
            Uint8* pixel = row + x * bpp;
            Uint32 color = *(Uint32*)pixel;
            Uint8 r, g, b, a;
            SDL_GetRGBA(color, format, &r, &g, &b, &a);
            if (a == 255) map[y][x] = 1;
        }
        
    }
    return map;
}

int main(int argc, char* argv[]) {
    char* base_path = SDL_GetBasePath();
    fs::current_path(base_path);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL init error: %s", SDL_GetError());
        return 1;
    }
    if (!IMG_Init(IMG_INIT_PNG)) {
        SDL_Log("IMG init error: %s", IMG_GetError());
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)<0) {
        SDL_Log("SDL mixer error: %s", Mix_GetError());
    }
    SDL_Window* window = SDL_CreateWindow(
        "Platformer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        600,
        600,
        SDL_WINDOW_SHOWN
    );
    SDL_Color BLACK = {0,0,0,255};
    SDL_Color WHITE = {255,255,255,255};
    SDL_Color TRANSPARENT = {0,0,0,0};
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Event event;
    SDL_Rect player = {280, 320, 40, 80};
    int vel_y = 0;
    int vel_x = 0;
    int start = SDL_GetTicks();
    int other_start;
    SDL_Surface* character = IMG_Load("assets/character.png");
    SDL_Surface* level_surf = IMG_Load("assets/levels/0,0.png");
    SDL_Surface* spikes_surf = IMG_Load("assets/levels/0,0s.png");
    SDL_Texture* character_te = SDL_CreateTextureFromSurface(renderer, character);
    SDL_Texture* level_te = SDL_CreateTextureFromSurface(renderer, level_surf);
    SDL_Texture* spikes_te = SDL_CreateTextureFromSurface(renderer, spikes_surf);
    SDL_SetTextureBlendMode(character_te, SDL_BLENDMODE_BLEND);
    std::vector<std::vector<Uint8>> character_mask = create_mask(character);
    std::vector<std::vector<Uint8>> level_mask = create_mask(level_surf);
    std::vector<std::vector<Uint8>> spikes_mask = create_mask(spikes_surf);
    SDL_FreeSurface(character);
    SDL_FreeSurface(level_surf);
    SDL_FreeSurface(spikes_surf);
    int rj = 0;
    bool do_vel;
    const Uint8* state;
    int aph = 255;
    std::string basepath = "assets/";
    SDL_Color color = {255,255,255,255};
    SDL_Color bg_color = {0,0,0,255};
    spritesheet sheet(16, 16, "!\"#$%&\'()*+,-./0\n123456789:;<=>?@\nABCDEFGHIJKLMNOP\nQRSTUVWXYZ[\\]^_ \n`abcdefghijklmno\npqrstuvwxyz{|}~\x7f", "assets/font.png", color, bg_color, 1.9);
    SDL_Surface* text = sheet.render("LEVEL 1: Welcome", &BLACK, &TRANSPARENT);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text);
    int w = text->w, h = text->h;
    SDL_Rect dst = {0,0,w,h};
    std::vector<int> room = {0,0};
    set_center({350, 80}, &dst);
    SDL_FreeSurface(text);
    ROOM_CHANGED = SDL_RegisterEvents(1);
    std::string path;
    std::string spike_path;
    Mix_Music* background = Mix_LoadMUS("assets/background_music.ogg");
    if (!background) {
        SDL_Log("LOAD MUSIC FAILED: %s", Mix_GetError());
    }
    if (Mix_PlayMusic(background, -1) == -1) {
        SDL_Log("PLAY MUSIC FAILED: %s", Mix_GetError());
    }
    goto game;
    death:
            SDL_Log("dying");
            other_start = SDL_GetTicks();
                while (true) {
                    if (((SDL_GetTicks() - other_start) % 10) == 0) if (aph != 0) aph -= 1;
                    if ((SDL_GetTicks() - other_start) >= 4000) break;
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT) {
                            SDL_DestroyTexture(texture);
                            SDL_DestroyTexture(character_te);
                            SDL_DestroyTexture(level_te);
                            SDL_DestroyRenderer(renderer);
                            SDL_DestroyWindow(window);
                            Mix_FreeMusic(background);
                            Mix_CloseAudio();
                            IMG_Quit();
                            SDL_Quit();
                            return 0;
                        }
                    }
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderClear(renderer);
                    SDL_SetRenderDrawColor(renderer, 130, 0, 130, 255);
                    draw_map(renderer, level_te);
                    draw_map(renderer, spikes_te);
                    SDL_SetTextureAlphaMod(character_te, aph);
                    SDL_RenderCopy(renderer, character_te, NULL, &player);
                    SDL_RenderPresent(renderer);
                }
                player = {280, 320, 40, 80};
                vel_y = 0;
                vel_x = 0;
                start = SDL_GetTicks();
                rj = 0;
                aph = 255;
                SDL_SetTextureAlphaMod(character_te, aph);
    game:
    while (true) {
        if ((SDL_GetTicks() - start) >= 30) {
            start = SDL_GetTicks();
            do_vel = true;
        } else {
            do_vel = false;
        }
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_DestroyTexture(texture);
                SDL_DestroyTexture(level_te);
                SDL_DestroyTexture(character_te);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                Mix_CloseAudio();
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && !event.key.repeat && rj) {
                vel_y = -18;
                rj = 0;
            }
            if (event.type == ROOM_CHANGED) {
                SDL_DestroyTexture(level_te);
                level_surf = IMG_Load((basepath+"levels/"+join(room, ",")+".png").c_str());
                spikes_surf = IMG_Load((basepath+"levels/"+join(room, ",")+"s.png").c_str());
                level_te = SDL_CreateTextureFromSurface(renderer, level_surf);
                spikes_te = SDL_CreateTextureFromSurface(renderer, spikes_surf);
                level_mask = create_mask(level_surf);
                spikes_mask = create_mask(spikes_surf);
                SDL_FreeSurface(level_surf);
                SDL_FreeSurface(spikes_surf);
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 130, 0, 130, 255);
        draw_map(renderer, level_te);
        draw_map(renderer, spikes_te);
        state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_LEFT]) {
            if (do_vel) vel_x -= 3;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            if (do_vel) vel_x += 3;
        }
        
        if (do_vel) {
            vel_y += 1;
            switch (move_x(vel_x, &player, character_mask, &level_mask, &room, &spikes_mask)) {
                case 's':
                    SDL_Log("move_x triggers death");
                    goto death;
                case 'r':
                    SDL_Log("room changed: move_x");
                    continue;
            }
            vel_x *= 0.8;
            if (abs(vel_x) < 0.1) vel_x = 0;    
            switch (move_y(vel_y, &player, &vel_y, &rj, character_mask, &level_mask, &room, &spikes_mask)) {
            case 's':
                SDL_Log("move_y triggers death");
                goto death;
            case 'r':
                continue;
            }
        }
        SDL_RenderCopy(renderer, character_te, NULL, &player);
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_RenderPresent(renderer);
        }
        
}