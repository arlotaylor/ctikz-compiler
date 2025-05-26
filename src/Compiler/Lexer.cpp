#include "Lexer.h"
#include "Assertion.h"

std::vector<Token> Tokenize(std::string str)
{
    str += " ";
    std::vector<Token> ret;
    int pos = 0;
    int line = 1; int lineStart = -1;  // will give weird results if carridge returns or other invisible characters are present, but oh well

    while (pos < str.size())
    {
        if (std::isspace(str[pos]))
        {
            if (str[pos] == '\n')
            {
                line += 1; lineStart = pos;
            }
            pos += 1;
        }
        else if (std::isalpha(str[pos]))
        {
            int begin = pos;
            do { pos += 1; } while (std::isalnum(str[pos]) || str[pos] == '_');

            std::string val = str.substr(begin, pos - begin);
            ret.push_back({ (val == "true" || val == "false") ? TokenType::Boolean : TokenType::Text, val, { line, begin - lineStart } });
        }
        else if (str[pos] == '"')
        {
            int begin = pos;
            std::string r = "";
            pos += 1;
            for (; pos < str.size(); pos++)
            {
                if (str[pos] == '"') break;

                if (str[pos] == '\\')
                {
                    pos += 1;
                    Assert(pos < str.size(), "End of file reached while lexing string.", { line, begin - lineStart });

                    size_t v = std::string("abfnrtv\\\'\"").find(str[pos]);
                    Assert(v != std::string::npos, "Unrecognized escape sequence.", { line, begin - lineStart });
                    r += std::string("\a\b\f\n\r\t\v\\\'\"")[v];
                }
                else
                {
                    r += str[pos];
                }
            }
            Assert(pos < str.size(), "End of file reached while lexing string.", { line, begin - lineStart });
            pos += 1;
            ret.push_back({ TokenType::StringLiteral, r, { line, begin - lineStart } });
        }
        else if (std::isdigit(str[pos]))
        {
            int begin = pos;
            do { pos += 1; } while (std::isdigit(str[pos]));
            if (str[pos] == '.')
            {
                do { pos += 1; } while (std::isdigit(str[pos]));
                ret.push_back({ TokenType::Decimal, str.substr(begin, pos - begin), { line, begin - lineStart } });
            }
            else
            {
                ret.push_back({ TokenType::Integer, str.substr(begin, pos - begin), { line, begin - lineStart } });
            }
        }
        else if (str[pos] == '.' && std::isdigit(str[pos + 1]))
        {
            int begin = pos; pos += 2;
            while (std::isdigit(str[pos])) pos += 1;
            ret.push_back({ TokenType::Decimal, str.substr(begin, pos - begin), { line, begin - lineStart } });
        }
        else if (str[pos] == '/' && str[pos + 1] == '/')  // comments
        {
            while (pos < str.size() && str[pos] != '\n') pos += 1;
        }
        else if (std::string("+-*/%^=!><").find(str[pos]) != std::string::npos)
        {
            int begin = pos; pos += 1;
            if (str[pos] == '=') pos += 1;
            ret.push_back({ TokenType::Symbol, str.substr(begin, pos - begin), { line, begin - lineStart } });
        }
        else if (std::string("&|").find(str[pos]) != std::string::npos)
        {
            int begin = pos; pos += 1;
            if (str[pos] == str[pos - 1]) pos += 1;
            ret.push_back({ TokenType::Symbol, str.substr(begin, pos - begin), { line, begin - lineStart } });
        }
        else if (std::string(",()[]{}_:;").find(str[pos]) != std::string::npos)
        {
            int begin = pos; pos += 1;
            ret.push_back({ TokenType::Symbol, str.substr(begin, pos - begin), { line, begin - lineStart } });
        }
        else
        {
            Assert(false, "Unrecognized symbol." + str.substr(pos), { line, pos - lineStart });
        }
    }

    ret.push_back({ TokenType::EndOfFile, "", { line, pos - lineStart } });
    return ret;
}

