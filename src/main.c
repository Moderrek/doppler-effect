#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

// Display
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PIXELS_PER_METER 3
// Waves
#define SOUND_SPEED 340.0f
#define MAX_WAVE_COUNT 500
// Audio
#define MAX_SAMPLES 512
#define MAX_SAMPLES_PER_UPDATE 4096

typedef struct {
  Vector2 position;
  Vector2 velocity;
  float frequency;
} SoundSource;

typedef struct {
  Vector2 position;
  Vector2 velocity;
} Observer;

typedef struct {
  Vector2 source;
  float distance; // distance from source
  bool exists;
  Color color;
} SoundWave;

float meter(const float meter) {
  return meter * PIXELS_PER_METER;
}

float sound_frequency = 0.0f;
float sound_frequency_queue = 0.0f;
float sineIdx = 0.0f;
float volume = 1.0f;

void AudioInputCallback(void *buffer, const unsigned int frames) {
  short *data = buffer;
  const float incr = sound_frequency / 48000.0f;
  for (unsigned int i = 0; i < frames; ++i) {
    data[i] = (short) (32000.0f * sinf(2 * PI * sineIdx));
    sineIdx += incr;
    if (sineIdx > 1.0f) sineIdx -= 1.0f;
  }
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Doppler Effect - Moderr");
  InitAudioDevice();
  SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);

  const AudioStream audio_stream = LoadAudioStream(48000, 16, 1);
  SetAudioStreamCallback(audio_stream, AudioInputCallback);
  short *data = (short *) malloc(sizeof(short) * MAX_SAMPLES);
  short *write_buffer = (short *) malloc(sizeof(short) * MAX_SAMPLES_PER_UPDATE);
  PlayAudioStream(audio_stream);

  SoundSource source = {
    (Vector2){100, 200},
    (Vector2){0 * 2, 0},
    0
  };

  Observer observer = {
    (Vector2){300, 200},
    (Vector2){0, 0},
  };

  SoundWave sound_waves[MAX_WAVE_COUNT] = {0};
  bool heard_waves[MAX_WAVE_COUNT] = {0};

  SetTargetFPS(60);

  float timer = 0.0f;
  float timeScale = 0.3f;
  float second_timer = 0.0f;
  while (!WindowShouldClose()) {
    const float deltaTime = GetFrameTime() * timeScale;
    timer += deltaTime;
    second_timer += deltaTime;

    // Controls
    if (IsKeyDown(KEY_UP)) source.frequency += 1;
    if (IsKeyDown(KEY_DOWN)) source.frequency -= 1;

    if (IsKeyDown(KEY_RIGHT)) source.velocity.x += SOUND_SPEED / 10;
    if (IsKeyDown(KEY_LEFT)) source.velocity.x -= SOUND_SPEED / 10;

    if (IsKeyDown(KEY_D)) observer.velocity.x += SOUND_SPEED / 10;
    if (IsKeyDown(KEY_A)) observer.velocity.x -= SOUND_SPEED / 10;
    if (IsKeyDown(KEY_W)) observer.velocity.y += SOUND_SPEED / 10;
    if (IsKeyDown(KEY_S)) observer.velocity.y -= SOUND_SPEED / 10;

    if (IsKeyDown(KEY_Z)) timeScale -= 0.1f;
    if (IsKeyDown(KEY_X)) timeScale += 0.1f;

    // Move bodies
    source.position = Vector2Add(source.position, Vector2Multiply(source.velocity, (Vector2){deltaTime, deltaTime}));
    observer.position = Vector2Add(observer.position,
                                   Vector2Multiply(observer.velocity, (Vector2){deltaTime, deltaTime}));

    // Move sound waves
    const float d = SOUND_SPEED * deltaTime;
    for (size_t i = 0; i < MAX_WAVE_COUNT; ++i) {
      if (!sound_waves[i].exists) continue;
      sound_waves[i].distance += d;
      if (sound_waves[i].distance > 800) {
        sound_waves[i].exists = false;
      }
    }

    // Emit sound waves
    if (source.frequency == 0) {
      timer = 0;
    }
    if (timer > 1 / source.frequency) {
      timer -= 1 / source.frequency;
      for (size_t i = 0; i < MAX_WAVE_COUNT; ++i) {
        if (sound_waves[i].exists) continue;
        sound_waves[i].distance = 0;
        sound_waves[i].source = source.position;
        sound_waves[i].exists = true;
        sound_waves[i].color = BLACK;
        break;
      }
    }

    // Update heard waves
    for (size_t i = 0; i < MAX_WAVE_COUNT; ++i) {
      if (!sound_waves[i].exists) continue;
      const float distance = floor(Vector2Distance(sound_waves[i].source, observer.position));
      const float wave_r = floor(sound_waves[i].distance);
      if (wave_r >= distance + 5) {
        sound_waves[i].color = BLUE;
        continue;
      }
      if (heard_waves[i]) continue;
      if (wave_r >= distance) {
        sound_frequency_queue++;
        sound_waves[i].color = RED;
        heard_waves[i] = true;
      }
    }

    // Update sound frequency
    if (second_timer > 0.1f) {
      second_timer -= 0.1f;
      // Every second
      for (size_t i = 0; i < MAX_WAVE_COUNT; ++i) {
        heard_waves[i] = false;
      }
      sound_frequency = sound_frequency_queue * 10;
      sound_frequency_queue = 0;
    }

    // Update title
    char window_title[40];
    sprintf(window_title, "Doppler Effect - Moderr - FPS: %d", GetFPS());
    SetWindowTitle(window_title);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw source
    DrawCircleV(source.position, meter(1), RED);

    // Draw observer
    DrawCircleV(observer.position, meter(1), BLUE);

    // Draw waves
    for (size_t i = 0; i < MAX_WAVE_COUNT; ++i) {
      const SoundWave wave = sound_waves[i];
      if (!wave.exists) continue;
      DrawCircleLinesV(wave.source, meter(1 + (wave.distance / PI)), wave.color);
    }

    EndDrawing();
  }

  // Free audio stream
  free(data);
  free(write_buffer);
  UnloadAudioStream(audio_stream);
  CloseAudioDevice();

  // Close OpenGL context
  CloseWindow();
  return 0;
}
