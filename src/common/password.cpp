// Copyright (c) 2018, Bitlitas
// All rights reserved. Based on Cryptonote and Monero.

#include "password.h"

#include <iostream>
#include <memory.h>
#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef HAVE_READLINE
  #include "readline_buffer.h"
#endif

#include "memwipe.h"

namespace
{
#if defined(_WIN32)
  bool is_cin_tty() noexcept
  {
    return 0 != _isatty(_fileno(stdin));
  }

  bool read_from_tty(epee::wipeable_string& pass)
  {
    static constexpr const char BACKSPACE = 8;

    HANDLE h_cin = ::GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode_old;
    ::GetConsoleMode(h_cin, &mode_old);
    DWORD mode_new = mode_old & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    ::SetConsoleMode(h_cin, mode_new);

    bool r = true;
    pass.reserve(tools::password_container::max_password_size);
    while (pass.size() < tools::password_container::max_password_size)
    {
      DWORD read;
      char ch;
      r = (TRUE == ::ReadConsoleA(h_cin, &ch, 1, &read, NULL));
      r &= (1 == read);
      if (!r)
      {
        break;
      }
      else if (ch == '\n' || ch == '\r')
      {
        std::cout << std::endl;
        break;
      }
      else if (ch == BACKSPACE)
      {
        if (!pass.empty())
        {
          pass.pop_back();
        }
      }
      else
      {
        pass.push_back(ch);
      }
    }

    ::SetConsoleMode(h_cin, mode_old);

    return r;
  }

#else // end WIN32 

  bool is_cin_tty() noexcept
  {
    return 0 != isatty(fileno(stdin));
  }

  int getch() noexcept
  {
    struct termios tty_old;
    tcgetattr(STDIN_FILENO, &tty_old);

    struct termios tty_new;
    tty_new = tty_old;
    tty_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_new);

    int ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &tty_old);

    return ch;
  }

  bool read_from_tty(epee::wipeable_string& aPass)
  {
    static constexpr const char BACKSPACE = 127;

    aPass.reserve(tools::password_container::max_password_size);
    while (aPass.size() < tools::password_container::max_password_size)
    {
      int ch = getch();
      if (EOF == ch)
      {
        return false;
      }
      else if (ch == '\n' || ch == '\r')
      {
        std::cout << std::endl;
        break;
      }
      else if (ch == BACKSPACE)
      {
        if (!aPass.empty())
        {
          aPass.pop_back();
        }
      }
      else
      {
        aPass.push_back(ch);
      }
    }

    return true;
  }

#endif // end !WIN32

  bool read_from_tty(const bool verify, const char *message, epee::wipeable_string& pass1, epee::wipeable_string& pass2)
  {
    while (true)
    {
      if (message)
        std::cout << message <<": ";
      if (!read_from_tty(pass1))
        return false;
      if (verify)
      {
        std::cout << "Confirm password: ";
        if (!read_from_tty(pass2))
          return false;
        if(pass1!=pass2)
        {
          std::cout << "Passwords do not match! Please try again." << std::endl;
          pass1.clear();
          pass2.clear();
        }
        else //new password matches
          return true;
      }
      else
        return true;
        //No need to verify password entered at this point in the code
    }

    return false;
  }

  bool read_from_file(epee::wipeable_string& pass)
  {
    pass.reserve(tools::password_container::max_password_size);
    for (size_t i = 0; i < tools::password_container::max_password_size; ++i)
    {
      char ch = static_cast<char>(std::cin.get());
      if (std::cin.eof() || ch == '\n' || ch == '\r')
      {
        break;
      }
      else if (std::cin.fail())
      {
        return false;
      }
      else
      {
        pass.push_back(ch);
      }
    }
    return true;
  }

} // anonymous namespace

namespace tools 
{
  // deleted via private member
  password_container::password_container() noexcept : m_password() {}
  password_container::password_container(std::string&& password) noexcept
    : m_password(std::move(password)) 
  {
  }

  password_container::~password_container() noexcept
  {
    m_password.clear();
  }

  boost::optional<password_container> password_container::prompt(const bool verify, const char *message)
  {
    password_container pass1{};
    password_container pass2{};
    if (is_cin_tty() ? read_from_tty(verify, message, pass1.m_password, pass2.m_password) : read_from_file(pass1.m_password))
      return {std::move(pass1)};

    return boost::none;
  }

  boost::optional<login> login::parse(std::string&& userpass, bool verify, const std::function<boost::optional<password_container>(bool)> &prompt)
  {
    login out{};

    const auto loc = userpass.find(':');
    if (loc == std::string::npos)
    {
      auto result = prompt(verify);
      if (!result)
        return boost::none;

      out.password = std::move(*result);
    }
    else
    {
      out.password = password_container{userpass.substr(loc + 1)};
    }

    out.username = userpass.substr(0, loc);
    password_container wipe{std::move(userpass)};
    return {std::move(out)};
  }
} 