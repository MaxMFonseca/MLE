#pragma once

#include <utility>

#include "Types.h"

namespace mle::ui {
class Controller {
  public:
    Controller(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller& operator=(Controller&&) = delete;

    Controller() = default;
    virtual ~Controller() = default;

    virtual void init() = 0;
    virtual void update() = 0;

    virtual void registerLua() {};
    virtual void unregisterLua() {};

  protected:
};

}  // namespace mle::ui

// TODO: implement this
// class LuaController : public Controller {
//   public:
//   private:
// };
