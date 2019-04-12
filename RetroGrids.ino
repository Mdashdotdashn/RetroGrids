#include <RK002.h>
#include "grids.h"
 
RK002_DECLARE_INFO("RETRO GRIDS","marc.nostromo@gmail.com","1.0","b9dad8cd-b032-4dca-9b44-8b4486880e8c9")

using namespace grids;

constexpr static uint8_t kMidiChannel = 9;

static uint32_t random_value(std::uint32_t init = 0) {
  static std::uint32_t val = 0x12345;
  if (init) {
    val = init;
    return 0;
  }
  val = val * 214013 + 2531011;
  return val;
}

enum Resolutions
{
  DOUBLE = 0,
  NORMAL,
  HALF,
  COUNT
};

static const uint8_t notes[3] = { 0x3C, 0x3E, 0x40 };

class GridsWrapper
{
public:
  GridsWrapper()
  {
    tickCount_ = 0;
    for (auto i = 0; i < 3; i ++)
    {
      channelTriggered_[i] = false;
      density_[i] = i == 0 ? 128 : 0;
      perturbations_[i] = 0;
    }
    x_ = y_ = 128;

    accent_ = 128;
    chaos_ = 0;
    divider_ = 1;
    multiplier_ = 1;
    running_ = false;
  }

  void start()
  {
    tickCount_ = 0;
    running_ = true;
  }

  void stop()
  {
    running_ = false;
  }

  void proceed()
  {
    running_ = true;
  }
  
  void tick()
  {
    if (!running_) return;

    uint32_t ticksPerClock = 3 << divider_;
    bool trigger = ((tickCount_ % ticksPerClock) == 0);

    if (trigger)
    {
      const auto step = (tickCount_ / ticksPerClock * multiplier_) % grids::kStepsPerPattern;
      channel_.setStep(step);

      for (auto channel = 0; channel < 3; channel++)
      {
        if (step == 0)
        {
          std::uint32_t r = random_value();
          perturbations_[channel] = ((r & 0xFF) * (chaos_ >> 2)) >> 8;
        }

        const uint8_t threshold = ~density_[channel];
        auto level = channel_.level(channel, x_, y_);
        if (level < 255 - perturbations_[channel])
        {
          level += perturbations_[channel];
        }

        if (level > threshold)
        {
          uint8_t targetLevel =  uint8_t(127.f * float(level - threshold) / float(256 - threshold));
          uint8_t noteLevel = grids::U8Mix(127, targetLevel, accent_);
          RK002_sendNoteOn(kMidiChannel, notes[channel], noteLevel);
          channelTriggered_[channel] = true;
        }    
      }
    }
    else
    {
      for (auto channel = 0; channel < 3; channel++)
      {
        if (channelTriggered_[channel])
        {
          RK002_sendNoteOff(kMidiChannel, notes[channel], 0);
          channelTriggered_[channel] = false;          
        }
      }
    }
    
    tickCount_++;
  }

  void setDensity(uint8_t channel, uint8_t density)
  {
    density_[channel] = density;
  }
    
  void setX(uint8_t x)
  {
    x_ = x;
  }

  void setY(uint8_t y)
  {
    y_ = y;
  }

  void setChaos(uint8_t c)
  {
    chaos_ = c;
  }

  void setResolution(Resolutions r)
  {
    multiplier_ = (r == DOUBLE) ? 2 : 1;
    divider_ = (r == HALF) ? 2 : 1;
  }

  void setAccent(uint8_t a)
  {
    accent_ = a;
  }

private:
 Channel channel_;
 uint32_t divider_;
 uint8_t multiplier_;
 uint32_t tickCount_;
 uint8_t density_[3];
 uint8_t perturbations_[3];
 uint8_t x_;
 uint8_t y_;
 bool channelTriggered_[3];
 uint8_t chaos_;
 uint8_t accent_;
 bool running_;  
};

static GridsWrapper wrapper;

bool RK002_onClock()
{
  wrapper.tick();
  return true;
}

bool RK002_onStart()
{
  wrapper.start();
  return true;  
}

bool RK002_onStop()
{
  wrapper.stop();
  return true;  
}

bool RK002_onContinue()
{
  wrapper.proceed();
  return true;  
}

bool RK002_onNoteOn(byte channel, byte key, byte velocity)
{
  return channel != 9;
}

boolean RK002_onControlChange(byte channel, byte nr, byte value)
{
  if (channel == 0)
  {
    switch(nr)
    {
      case 0x50:
        wrapper.setX(uint8_t(value) << 1);
        break;
      case 0x51:
        wrapper.setY(uint8_t(value) << 1);
        break;
      case 0x52:
        wrapper.setDensity(0, uint8_t(value) << 1);
        break;
      case 0x53:
        wrapper.setDensity(1, uint8_t(value) << 1);
        break;
      case 0x54:
        wrapper.setDensity(2, uint8_t(value) << 1);
        break;
      case 0x55:
        wrapper.setChaos(uint8_t(value) << 1);
        break;
      case 0x56:
        wrapper.setAccent(uint8_t(value) << 1);
        break;
      case 0x57:
        wrapper.setResolution(Resolutions((float(value) / 128.f) * Resolutions::COUNT));
        break;
    }
    return false;
  }
  return true;
}

void setup()
{
}
 
void loop()
{
}
