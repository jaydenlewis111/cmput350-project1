#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <SFML/Window/Keyboard.hpp>
namespace CMPUT350 {
// The assignment's supplied Keyboard.h was absent. This adapter names SFML's
// physical scancodes; convert Key to int when installing a key notification.
using Key = sf::Keyboard::Scancode;
}
#endif
