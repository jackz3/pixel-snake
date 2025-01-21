#define OLC_PGE_APPLICATION
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "olcPixelGameEngine.h"

#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef __EMSCRIPTEN__
#define MA_NO_DECODING
#include <emscripten.h>

void main_loop__em()
{
}
#endif

template<typename T>
void log(const char* txt, T info) {
  std::cout << txt << " " << info << std::endl;
}

namespace snake {
struct sSegment
{
	int x;
	int y;
};
enum Direction {
  UP, RIGHT, DOWN, LEFT
};
enum FoodType {
  GreenApple, RedApple, GoldApple, Egg
};
const int gridWidth = 32;
const int gridHeight = 24;
const int gridSize = 8;
olc::vf2d gridSize2d = { (float)gridSize, (float)gridSize };
olc::vf2d sprSize = { 8.0f, 8.0f };

olc::vf2d getVf2d(const sSegment& ss) {
  return olc::vf2d{ (float)gridSize * ss.x, (float)gridSize * ss.y};
}

class Sound {
public:
  Sound() {
    ma_result result;
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
      printf("Failed to initialize audio engine.");
    }
  }
  void Play(std::string file) {
    ma_engine_play_sound(&engine, ("./assets/" + file + ".mp3").c_str(), NULL);
  }
  ~Sound() {
    ma_engine_uninit(&engine);
  }
private:
  ma_engine engine;
};
Sound sound;

class Cherry {
public:
  olc::vf2d pic = olc::vf2d{ 48.0f, 24.0f };
  sSegment pos;
  int ToBeEaten() {
    return ++score_;
  }
  olc::vf2d get_pos() {
    return getVf2d(pos);
  }
  void Draw(float fElapsedTime, olc::Decal* decalPicPtr, olc::PixelGameEngine* pge) {
    pge->DrawPartialDecal(getVf2d(pos), gridSize2d, decalPicPtr, pic, sprSize);
  }
  int get_score() {
    return score_;
  }
private:
  int score_ = 1;
};

class VarFood {
public:
  void Reset(sSegment pos) {
    type_ = GetRndFood();
    pos_ = pos;
    visible_ = true;
    duration_ = 0.0f;
    switch (type_) {
      case GreenApple:
        score_ = 2;
        pic = olc::vf2d{ 48.0f, 32.0f };
      break;
      case GoldApple:
        score_ = 5;
        pic = olc::vf2d{ 48.0f, 8.0f };
      break;
      case RedApple:
        score_ = 10;
        pic = olc::vf2d{ 48.0f, 0.0f };
      break;
      case Egg:
        score_ = 0;
        pic = olc::vf2d{ 56.0f, 24.0f };
    }
  }
  bool Match(sSegment p) {
    return p.x == pos_.x && p.y == pos_.y;
  }
  void Draw(float fElapsedTime, olc::Decal* decalPicPtr, olc::PixelGameEngine* pge) {
    if(visible_) {
      duration_ += fElapsedTime;
      float diff = duration_ - stable_duration_;
      if (diff > 0) {
        if(diff > 8.0f) {
          visible_ = false;
          return;
        }
        if(((int)(diff * 10) % 10) < 6) {
          pge->DrawPartialDecal(getVf2d(pos_), gridSize2d, decalPicPtr, pic, sprSize);
        }
      } else {
        pge->DrawPartialDecal(getVf2d(pos_), gridSize2d, decalPicPtr, pic, sprSize);
      }
    }
  }
  bool get_visible() { return visible_; }
  void set_visible(bool v) { visible_ = v; }
  int get_score() { return score_; }
  auto get_type() { return type_; }
private:
  FoodType type_;
  sSegment pos_;
  bool visible_ = false;
  int score_;
  float duration_ = 0.0f;
  float stable_duration_ = 8.0f;
  olc::vf2d pic = olc::vf2d{ 48.0f, 24.0f };
  FoodType GetRndFood() {
    int r = rand() % 10;
    log("var food", r);
    if (r < 5) {
      return GreenApple;
    } else if (r < 8) {
      return GoldApple;
    } else if (r < 9) {
      return RedApple;
    } else {
      return Egg;
    }
  }
};

class Snake {
public:
  void Reset() {
    snake.clear();
    SetDefaultSkin();
    int startX = gridWidth / 2;
    int startY = gridHeight / 2;
    for (int i = 0; i < 5; i++) {
      snake.push_back({ startX + i, startY});
    }
    snakeDirection = LEFT;
    isDead_ = false;
  }
  void Move() {
    switch (snakeDirection)
      {
      case UP:
        snake.insert(snake.begin(), { snake.front().x, snake.front().y - 1 });
        break;
      case RIGHT:
        snake.insert(snake.begin(), { snake.front().x + 1, snake.front().y });
        break;
      case DOWN:
        snake.insert(snake.begin(), { snake.front().x, snake.front().y + 1 });
        break;
      case LEFT:
        snake.insert(snake.begin(), { snake.front().x - 1, snake.front().y });
        break;
      }
    snake.pop_back();
  }
  bool CheckOverlapping(int x, int y) {
    for (auto& ss: snake) {
      if (x == ss.x && y == ss.y) {
        return true;
      }
    }
    return false;
  }
  void Draw(olc::PixelGameEngine* pge, olc::Decal* decalPicPtr) {
    sSegment ss = snake.front();
    olc::vf2d pos = getVf2d(ss);
    pge->DrawPartialDecal(pos, gridSize2d, decalPicPtr, getSnakehead(), sprSize);
    for (int i = 1; i < snake.size(); i++) {
      pos = getVf2d(snake[i]);
      pge->DrawPartialDecal(pos, gridSize2d, decalPicPtr, body_pic_, sprSize);
    }
  }
  void ChangeSkin(FoodType ft) {
    sound.Play("med");
    switch (ft) {
      case GreenApple:
        up_pic_ = olc::vf2d{ 8.0f, 0.0f };
        right_pic_ = olc::vf2d{ 32.0f, 0.0f };
        down_pic_ = olc::vf2d{ 24.0f, 0.0f };
        left_pic_ = olc::vf2d{ 16.0f, 0.0f };
        body_pic_ = olc::vf2d{ 40.0f, 0.0f };
        break;
      case GoldApple:
        up_pic_ = olc::vf2d{ 8.0f, 32.0f };
        right_pic_ = olc::vf2d{ 32.0f, 32.0f };
        down_pic_ = olc::vf2d{ 24.0f, 32.0f };
        left_pic_ = olc::vf2d{ 16.0f, 32.0f };
        body_pic_ = olc::vf2d{ 40.0f, 32.0f };
        break;
      case RedApple:
        up_pic_ = olc::vf2d{ 8.0f, 24.0f };
        right_pic_ = olc::vf2d{ 32.0f, 24.0f };
        down_pic_ = olc::vf2d{ 24.0f, 24.0f };
        left_pic_ = olc::vf2d{ 16.0f, 24.0f };
        body_pic_ = olc::vf2d{ 40.0f, 24.0f };
        break;
      case Egg:
        up_pic_ = olc::vf2d{ 8.0f, 8.0f };
        right_pic_ = olc::vf2d{ 32.0f, 8.0f };
        down_pic_ = olc::vf2d{ 24.0f, 8.0f };
        left_pic_ = olc::vf2d{ 16.0f, 8.0f };
        body_pic_ = olc::vf2d{ 40.0f, 8.0f };
    }
  }
  bool ChangeHead(Direction direction) {
    if(direction == LEFT && (snakeDirection == DOWN || snakeDirection == UP)) {
      sound.Play("left");
      snakeDirection = LEFT;
      return true;
    }
    if(direction == RIGHT && (snakeDirection == DOWN || snakeDirection == UP)) {
      sound.Play("right");
      snakeDirection = RIGHT;
      return true;
    }
    if(direction == UP && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
      sound.Play("up");
      snakeDirection = UP;
      return true;
    }
    if(direction == DOWN && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
      sound.Play("down");
      snakeDirection = DOWN;
      return true;
    }
    return false;
  }
  void AddTail() {
    sound.Play("eat");
    snake.push_back(snake.back());
  }
  sSegment front() { return snake.front(); }
  void CheckBorderCOllision() {
    if (snake.front().x < 1 || snake.front().x >= gridWidth - 1) {
      isDead_ = true;
      sound.Play("dead");
    }
    if (snake.front().y < 1 || snake.front().y >= gridHeight - 1) {
      isDead_ = true;
      sound.Play("dead");
    }
  }
  void CheckSelfCollision() {
    sSegment* head = &snake[0];
    for (sSegment& s: snake) {
      if (head != &s && s.x == (*head).x && s.y == (*head).y) {
        isDead_ = true;
        sound.Play("dead");
        break;
      }
    }
  }
  olc::vf2d getSnakehead() {
    switch (snakeDirection) {
      case UP:
        return up_pic_;
      case RIGHT:
        return right_pic_;
      case DOWN:
        return down_pic_;
      case LEFT:
      default:
        return left_pic_;
    }
  }
  void set_direction(Direction dir) {
    snakeDirection = dir;
  }
  bool isDead() { return isDead_; }

private:
  std::vector<sSegment> snake = {};
  Direction snakeDirection = LEFT;
  bool isDead_ = false;
  int skin = 0;
  olc::vf2d up_pic_ = olc::vf2d{ 8.0f, 32.0f };
  olc::vf2d right_pic_ = olc::vf2d{ 32.0f, 32.0f };
  olc::vf2d down_pic_ = olc::vf2d{ 24.0f, 32.0f };
  olc::vf2d left_pic_ = olc::vf2d{ 16.0f, 32.0f };
  olc::vf2d body_pic_ = olc::vf2d{ 40.0f, 32.0f };
  void SetDefaultSkin() {
    up_pic_ = olc::vf2d{ 8.0f, 32.0f };
    right_pic_ = olc::vf2d{ 32.0f, 32.0f };
    down_pic_ = olc::vf2d{ 24.0f, 32.0f };
    left_pic_ = olc::vf2d{ 16.0f, 32.0f };
    body_pic_ = olc::vf2d{ 40.0f, 32.0f };
  }
};

class Game: public olc::PixelGameEngine {
private:
  std::unique_ptr<olc::Decal> decFragment;
  VarFood varFood;
  Cherry cherry;
  Snake snak;
  int nScore;
  std::chrono::milliseconds delay;
  std::chrono::system_clock::time_point t1;
  int stage = 0;

public:
  Game() {
		sAppName = "Pixel Snake";
  }
	bool OnUserCreate() override
	{
    srand(time(0));
    std::unique_ptr<olc::Sprite> sprFragment;
    sprFragment = std::make_unique<olc::Sprite>("./assets/snake.png");
    decFragment = std::make_unique<olc::Decal>(sprFragment.get());
		return true;
	}
	bool OnUserUpdate(float fElapsedTime) override
	{
    olc::Decal* decalPicPtr = decFragment.get();
    if (stage == 0) {
      return RenderTitle(fElapsedTime, decalPicPtr);
    }
    if (stage == 1) {
      return RenderGame(fElapsedTime, decalPicPtr);
    }
    return true;
	}
  bool RenderTitle(float fElapsedTime, olc::Decal* decalPicPtr) {
    drawWalls();
    DrawStringDecal(olc::vf2d{32.0f, 70.0f}, "PIXEL SNAKE", olc::YELLOW, olc::vf2d{2.2f, 2.5f});
    DrawStringDecal(olc::vf2d{66.0f, 120.0f}, "Press Enter to Start", olc::GREY, olc::vf2d{0.8f, 0.8f});
    if(GetKey(olc::Key::ENTER).bPressed) {
      Reset();
    }
    return true;
  }
  bool RenderGame(float fElapsedTime, olc::Decal* decalPicPtr) {
    DrawPartialDecal(cherry.get_pos(), gridSize2d, decalPicPtr, cherry.pic, sprSize);
    varFood.Draw(fElapsedTime, decalPicPtr, this);

    snak.Draw(this, decalPicPtr);
    drawWalls();
    DrawStringDecal(olc::vf2d{(float)(ScreenWidth() - 50), 2.0f}, "SCORE: " + std::to_string(nScore), olc::BLUE, olc::vf2d{0.6f, 0.5f});
    bool needUpdate = false;
    if(GetKey(olc::Key::LEFT).bPressed) {
      needUpdate = snak.ChangeHead(LEFT);
    }
    if(GetKey(olc::Key::RIGHT).bPressed) {
      needUpdate = snak.ChangeHead(RIGHT);
    }
    if(GetKey(olc::Key::UP).bPressed) {
      needUpdate = snak.ChangeHead(UP);
    }
    if(GetKey(olc::Key::DOWN).bPressed) {
      needUpdate = snak.ChangeHead(DOWN);
    }
    if (snak.isDead()) {
      DrawStringDecal(olc::vf2d{50.0f, 70.0f}, "GAME OVER", olc::WHITE, olc::vf2d{2.2f, 2.5f});
      DrawStringDecal(olc::vf2d{48.0f, 120.0f}, "Press Enter to Play Again!", olc::GREY, olc::vf2d{0.8f, 0.8f});
      if(GetKey(olc::Key::ENTER).bPressed) {
        Reset();
      }
      return true;
    }
    
    auto t2 = std::chrono::system_clock::now();
    if ((t2 - t1) < delay && !needUpdate) {
      return true;
    }

    t1 = t2;
    snak.Move();

    if (snak.front().x == cherry.pos.x && snak.front().y == cherry.pos.y) {
				nScore++;
        snak.AddTail();
        int cherryScore = cherry.ToBeEaten();
        cherry.pos = randPos();
        if (!varFood.get_visible() && cherryScore > 0 && cherryScore % 2 == 0) {
          varFood.Reset(randPos());
        }
        delay = std::chrono::milliseconds((int)(delay.count() * 0.95));
			}
    if (varFood.get_visible() && varFood.Match(snak.front())) {
      nScore += varFood.get_score();
      snak.ChangeSkin(varFood.get_type());
      snak.AddTail();
      varFood.set_visible(false);
    }

    snak.CheckBorderCOllision();
    snak.CheckSelfCollision(); 
		return true;
  }
  void Reset () {
    delay = std::chrono::milliseconds(500);
    nScore = 0;
    t1 = std::chrono::system_clock::now();
    snak.Reset();
    cherry.pos = randPos();
    varFood.set_visible(false);
    stage = 1;
  }

  sSegment randPos () {
    bool invalid = false;
    int x = 0;
    int y = 0;
    do {
      x = (rand() % (gridWidth - 2)) + 1;
      y = (rand() % (gridHeight - 2))+ 1;
      invalid = snak.CheckOverlapping(x, y);
    }
    while (invalid);
    // std::cout << x << "  " << y << std::endl;
    return sSegment { x, y };
  }
  void drawWalls() {
    olc::Decal* picPtr = decFragment.get();
    olc::vf2d pos = { 0.f, 0.f };
    DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 13.0f * gridSize, 0.0f }, sprSize);
    for (int i = 1; i < gridWidth-1; i++) {
      pos.x = i * gridSize;
      DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 56.f, 16.f }, sprSize);
    }
    pos.x += (float)gridSize;
    DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 12.0f * gridSize, 0.0f }, sprSize);
    pos.y = (gridHeight - 1) * gridSize;
    pos.x = 0;
    DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 13.0f * gridSize, 0.0f }, sprSize);
    for (int i = 1; i < gridWidth-1; i++) {
      pos.x = i * gridSize;
      DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 80.0f, 16.0f }, sprSize);
    }
    pos.x += (float)gridSize;
    DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 12.0f * gridSize, 0.0f }, sprSize);
    pos.x = 0; 
    for (int i = 1; i < gridHeight-1; i++) {
      pos.y = i * gridSize;
      DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 64.0f, 16.0f }, sprSize);
    }
    pos.x = (gridWidth - 1) * gridSize; 
    for (int i = 1; i < gridHeight-1; i++) {
      pos.y = i * gridSize;
      DrawPartialDecal(pos, gridSize2d, picPtr, olc::vf2d{ 72.0f, 16.0f }, sprSize);
    }
  }

  void run() {
    if(Construct(gridWidth * gridSize, gridHeight * gridSize, 4, 4)) {
      Start();
    }
  }
};
} //namespace snake

int main () {
	snake::Game game;
  game.run();
}
