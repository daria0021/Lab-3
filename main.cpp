#include <iostream>   // Подключение библиотеки для ввода/вывода (cout, cin)
#include <string>     // Подключение библиотеки для работы со строками (string)
#include <vector>     // Подключение библиотеки для использования динамических массивов (vector)
#include <map>        // Подключение библиотеки для ассоциативных массивов (map)
#include <cctype>     // Подключение библиотеки для функций проверки символов (isdigit, isalpha, tolower)
#include <sstream>    // Подключение библиотеки для строковых потоков (ostringstream, istringstream)
#include <iomanip>    // Подключение библиотеки для манипуляторов форматирования ввода/вывода
#include <cmath>      // Подключение математической библиотеки (sin, cos, exp, log, sqrt, pow, isinf, isnan)
#include <memory>     // Подключение библиотеки для умных указателей (unique_ptr)
#include <algorithm>  // Подключение библиотеки для алгоритмов (transform)
#include <set>        // Подключение библиотеки для множеств (set)
#include <stdexcept>  // Подключение библиотеки для стандартных исключений (runtime_error)

using namespace std;  // Использование пространства имён std, чтобы не писать std:: перед стандартными функциями
// компиляция кода: g++ -std=c++20 -o CoMEWD main.cpp      ./CoMEWD

// ============================================================================
// Вспомогательные функции
// ============================================================================

string to_lower(const string& s) {  // Функция преобразования строки в нижний регистр
    string res = s;                  // Создание копии исходной строки
    transform(res.begin(), res.end(), res.begin(), ::tolower);  // Применение tolower к каждому символу строки
    return res;                      // Возврат преобразованной строки
}

// Форматирование числа для вывода в сообщениях об ошибках
string format_double_for_error(double d) {  // Функция форматирования double для сообщений об ошибках
    ostringstream oss;                       // Создание строкового потока для форматирования
    oss << d;                                // Запись числа в поток
    string s = oss.str();                    // Преобразование потока в строку
    if (s.find('.') != string::npos) {       // Если в строке есть десятичная точка
        s.erase(s.find_last_not_of('0') + 1, string::npos);  // Удаление всех конечных нулей
        if (s.back() == '.') s.pop_back();   // Если последний символ стал точкой, удалить её
    }
    return s;                                // Возврат отформатированной строки
}

// Вывод числа с удалением лишних нулей
void print_double(double d) {                // Функция вывода числа без лишних нулей
    if (isinf(d)) {                          // Если число является бесконечностью
        if (d > 0)                           // Если положительная бесконечность
            cout << "inf";                   // Вывод "inf"
        else                                 // Если отрицательная бесконечность
            cout << "-inf";                  // Вывод "-inf"
        return;                              // Выход из функции
    }
    if (isnan(d)) {                          // Если число является NaN (Not a Number)
        cout << "nan";                       // Вывод "nan"
        return;                              // Выход из функции
    }

    // Используем стандартный вывод double
    ostringstream oss;                       // Создание строкового потока
    oss << d;                                // Запись числа в поток
    string s = oss.str();                    // Преобразование в строку

    // Удаляем конечные нули, но оставляем хотя бы один знак после запятой если есть
    if (s.find('.') != string::npos) {       // Если есть десятичная точка
        s.erase(s.find_last_not_of('0') + 1, string::npos);  // Удаление конечных нулей
        if (s.back() == '.') s.pop_back();   // Удаление точки если она осталась последней
    }

    cout << s;                               // Вывод отформатированной строки
}

bool is_function_name(const string& name) {  // Функция проверки, является ли строка именем встроенной функции
    static const set<string> functions = {   // Статическое множество имён допустимых функций
        "sin", "cos", "tan", "asin", "acos", "atan",
        "exp", "log", "sqrt"
    };
    return functions.count(to_lower(name)) > 0;  // Возврат true если имя есть в множестве (после приведения к нижнему регистру)
}

// ============================================================================
// Токены
// ============================================================================

enum class TokenType {    // Перечисление типов токенов
    NUMBER,               // Числовой токен
    IDENTIFIER,           // Идентификатор (переменная или имя функции)
    OPERATOR,             // Оператор (+, -, *, /, ^)
    LPAREN,               // Левая круглая скобка
    RPAREN,               // Правая круглая скобка
    END                   // Конец входной строки
};

struct Token {            // Структура для хранения токена
    TokenType type;       // Тип токена
    double num_value;     // Значение для числового токена
    string str_value;     // Строковое значение для идентификатора и оператора
    string raw_value;     // Исходная строка для числа (для вывода в режиме лексера)
    size_t pos;           // Позиция токена в исходной строке

    Token(TokenType t, double val, const string& raw, size_t p)  // Конструктор для числового токена
        : type(t), num_value(val), raw_value(raw), pos(p) {}
    Token(TokenType t, const string& s, size_t p)                // Конструктор для идентификатора и оператора
        : type(t), str_value(s), pos(p) {}
    Token(TokenType t, size_t p) : type(t), pos(p) {}            // Конструктор для скобок и END
};

// ============================================================================
// Лексер
// ============================================================================

class Lexer {            // Класс лексера для разбора входной строки на токены
public:
    Lexer(const string& input) : input(input), pos(0) {}  // Конструктор: сохраняем строку и начальную позицию 0

    vector<Token> tokenize() {   // Основной метод лексера - разбор всей строки в вектор токенов
        vector<Token> tokens;    // Вектор для хранения токенов
        while (pos < input.size()) {  // Пока не достигнут конец строки
            char c = input[pos];      // Текущий символ
            if (isspace(c)) {         // Если символ - пробельный
                pos++;                // Пропускаем его
                continue;             // Переходим к следующему символу
            }
            if (isdigit(c)) {         // Если символ - цифра
                tokens.push_back(parse_number());  // Разбираем число и добавляем токен
            } else if (isalpha(c) || c == '_') {   // Если символ - буква или подчёркивание
                tokens.push_back(parse_identifier());  // Разбираем идентификатор и добавляем токен
            } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {  // Если символ - оператор
                tokens.emplace_back(TokenType::OPERATOR, string(1, c), pos);  // Создаём токен-оператор
                pos++;                     // Перемещаем позицию вперёд
            } else if (c == '(') {         // Если символ - левая скобка
                tokens.emplace_back(TokenType::LPAREN, pos);  // Создаём токен LPAREN
                pos++;                     // Перемещаем позицию вперёд
            } else if (c == ')') {         // Если символ - правая скобка
                tokens.emplace_back(TokenType::RPAREN, pos);  // Создаём токен RPAREN
                pos++;                     // Перемещаем позицию вперёд
            } else {                       // Если символ не распознан
                throw runtime_error("Unexpected character: " + string(1, c));  // Выбрасываем исключение
            }
        }
        tokens.emplace_back(TokenType::END, pos);  // Добавляем токен конца строки
        return tokens;                      // Возвращаем вектор токенов
    }

private:
    string input;      // Исходная входная строка
    size_t pos;        // Текущая позиция в строке

    Token parse_number() {          // Метод разбора числа
        size_t start = pos;         // Запоминаем начальную позицию
        bool leading_zero = false;  // Флаг для проверки ведущих нулей

        if (input[pos] == '0') {    // Если первый символ - ноль
            leading_zero = true;    // Устанавливаем флаг ведущего нуля
        }
        pos++;                      // Перемещаемся к следующему символу

        while (pos < input.size() && isdigit(input[pos])) {  // Пока есть цифры
            if (leading_zero) {      // Если был ведущий ноль и мы нашли ещё цифры
                throw runtime_error("Leading zeros not allowed in number");  // Ошибка: ведущие нули запрещены
            }
            pos++;                   // Перемещаемся к следующему символу
        }

        if (pos < input.size() && input[pos] == '.') {  // Если встретили десятичную точку
            pos++;                   // Перемещаемся после точки
            if (pos >= input.size() || !isdigit(input[pos])) {  // Если после точки нет цифры
                throw runtime_error("Expected digit after decimal point");  // Ошибка: ожидалась цифра
            }
            while (pos < input.size() && isdigit(input[pos])) {  // Читаем все цифры после точки
                pos++;
            }
        }

        if (pos < input.size() && (isalpha(input[pos]) || input[pos] == '_')) {  // Если после числа идёт буква
            throw runtime_error("Invalid character in number");  // Ошибка: недопустимый символ в числе
        }

        string num_str = input.substr(start, pos - start);  // Извлекаем подстроку числа
        double val;                                         // Переменная для хранения числового значения
        istringstream iss(num_str);                         // Создаём строковый поток из числа
        iss >> val;                                         // Преобразуем строку в double
        return Token(TokenType::NUMBER, val, num_str, start);  // Возвращаем числовой токен
    }

    Token parse_identifier() {      // Метод разбора идентификатора
        size_t start = pos;         // Запоминаем начальную позицию
        while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {  // Пока буква, цифра или подчёркивание
            pos++;                   // Перемещаемся вперёд
        }
        string ident = input.substr(start, pos - start);  // Извлекаем подстроку идентификатора
        return Token(TokenType::IDENTIFIER, ident, start);  // Возвращаем токен-идентификатор
    }
};

// ============================================================================
// Узлы AST (предварительные объявления)
// ============================================================================

class Node {           // Базовый абстрактный класс для всех узлов AST
public:
    virtual ~Node() = default;                                   // Виртуальный деструктор
    virtual double eval(const map<string, double>& vars) const = 0;  // Чисто виртуальный метод вычисления значения
    virtual Node* clone() const = 0;                             // Чисто виртуальный метод клонирования узла
    virtual Node* diff(const string& var) const = 0;             // Чисто виртуальный метод дифференцирования по переменной
    virtual string to_string() const = 0;                        // Чисто виртуальный метод строкового представления
    virtual bool is_constant() const { return false; }           // Виртуальный метод проверки константности (по умолчанию false)
};

class NumberNode;      // Предварительное объявление класса узла числа
class VariableNode;    // Предварительное объявление класса узла переменной
class UnaryOpNode;     // Предварительное объявление класса узла унарной операции
class BinaryOpNode;    // Предварительное объявление класса узла бинарной операции
class FunctionNode;    // Предварительное объявление класса узла функции

// ============================================================================
// NumberNode
// ============================================================================

class NumberNode : public Node {  // Класс узла для числовых констант
public:
    double value;                 // Значение числа

    NumberNode(double val) : value(val) {}  // Конструктор: инициализация значения

    double eval(const map<string, double>&) const override { return value; }  // Вычисление: возвращаем само число
    Node* clone() const override { return new NumberNode(value); }            // Клонирование: создаём новый NumberNode
    Node* diff(const string&) const override { return new NumberNode(0); }    // Производная от числа равна 0
    string to_string() const override {      // Строковое представление числа
        ostringstream oss;                   // Строковый поток
        oss << value;                        // Запись числа
        string s = oss.str();                // Преобразование в строку
        if (s.find('.') != string::npos) {   // Если есть десятичная точка
            s.erase(s.find_last_not_of('0') + 1, string::npos);  // Удаление конечных нулей
            if (s.back() == '.') s.pop_back();   // Удаление точки
        }
        return s;                            // Возврат отформатированной строки
    }
    bool is_constant() const override { return true; }  // Число всегда константа
};

// ============================================================================
// VariableNode
// ============================================================================

class VariableNode : public Node {  // Класс узла для переменной
public:
    string name;                    // Имя переменной

    VariableNode(const string& n) : name(to_lower(n)) {}  // Конструктор: сохраняем имя в нижнем регистре

    double eval(const map<string, double>& vars) const override {  // Вычисление значения переменной
        auto it = vars.find(name);            // Поиск переменной в map
        if (it == vars.end()) {               // Если переменная не найдена
            throw runtime_error("Unknown variable: " + name);  // Ошибка: неизвестная переменная
        }
        return it->second;                    // Возвращаем значение переменной
    }

    Node* clone() const override { return new VariableNode(name); }  // Клонирование: создаём новую переменную
    Node* diff(const string& var) const override {  // Производная по переменной
        return new NumberNode(name == to_lower(var) ? 1.0 : 0.0);  // 1 если по той же переменной, иначе 0
    }
    string to_string() const override { return name; }  // Строковое представление: имя переменной
    bool is_constant() const override { return false; } // Переменная не константа
};

// ============================================================================
// UnaryOpNode
// ============================================================================

class UnaryOpNode : public Node {  // Класс узла для унарных операций (пока только унарный минус)
public:
    char op;          // Операция (только '-')
    Node* child;      // Дочерний узел (операнд)

    UnaryOpNode(char op, Node* child) : op(op), child(child) {}  // Конструктор
    ~UnaryOpNode() { delete child; }    // Деструктор: освобождаем память дочернего узла

    double eval(const map<string, double>& vars) const override {  // Вычисление унарной операции
        double val = child->eval(vars);    // Вычисляем значение дочернего узла
        if (op == '-') return -val;        // Если операция '-', возвращаем отрицание
        return val;                        // Иначе (для '+') возвращаем как есть
    }

    Node* clone() const override { return new UnaryOpNode(op, child->clone()); }  // Клонирование

    Node* diff(const string& var) const override {  // Производная унарной операции
        if (op == '-') {                           // Если унарный минус
            return new UnaryOpNode('-', child->diff(var));  // Производная от -f(x) = -f'(x)
        } else {                                   // Если унарный плюс
            return child->diff(var);               // Производная от +f(x) = f'(x)
        }
    }

    string to_string() const override {            // Строковое представление
        if (op == '-') {                           // Для унарного минуса
            if (dynamic_cast<NumberNode*>(child) || dynamic_cast<VariableNode*>(child)) {  // Если операнд простой
                return "-" + child->to_string();   // Без скобок
            } else {                               // Если операнд сложный
                return "-(" + child->to_string() + ")";  // Со скобками
            }
        } else {                                   // Для унарного плюса
            return child->to_string();             // Просто возвращаем строку операнда
        }
    }
};

// ============================================================================
// BinaryOpNode (объявлен, определён после FunctionNode)
// ============================================================================

class BinaryOpNode : public Node {  // Класс узла для бинарных операций
public:
    char op;          // Операция (+, -, *, /, ^)
    Node* left;       // Левый операнд
    Node* right;      // Правый операнд

    BinaryOpNode(char op, Node* left, Node* right) : op(op), left(left), right(right) {}  // Конструктор
    ~BinaryOpNode() { delete left; delete right; }  // Деструктор: удаляем оба операнда

    double eval(const map<string, double>& vars) const override;  // Объявление метода вычисления (определение после FunctionNode)
    Node* clone() const override { return new BinaryOpNode(op, left->clone(), right->clone()); }  // Клонирование
    Node* diff(const string& var) const override;  // Объявление метода дифференцирования (определение после FunctionNode)
    string to_string() const override {  // Строковое представление в виде (левая операция правая)
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
    bool is_constant() const override { return left->is_constant() && right->is_constant(); }  // Константа если оба операнда константы
};

// ============================================================================
// FunctionNode
// ============================================================================

class FunctionNode : public Node {  // Класс узла для математических функций
public:
    string name;      // Имя функции (sin, cos, tan, asin, acos, atan, exp, log, sqrt)
    Node* arg;        // Аргумент функции

    FunctionNode(const string& n, Node* a) : name(to_lower(n)), arg(a) {}  // Конструктор
    ~FunctionNode() { delete arg; }  // Деструктор: удаляем аргумент

    double eval(const map<string, double>& vars) const override {  // Вычисление функции
        double x = arg->eval(vars);    // Вычисляем аргумент
        if (name == "sin") return sin(x);   // Синус
        if (name == "cos") return cos(x);   // Косинус
        if (name == "tan") return tan(x);   // Тангенс
        if (name == "asin") {               // Арксинус
            if (x < -1 || x > 1) {          // Проверка области определения
                throw runtime_error("Domain error: asin(" + format_double_for_error(x) + ")");
            }
            return asin(x);
        }
        if (name == "acos") {               // Арккосинус
            if (x < -1 || x > 1) {
                throw runtime_error("Domain error: acos(" + format_double_for_error(x) + ")");
            }
            return acos(x);
        }
        if (name == "atan") return atan(x);  // Арктангенс
        if (name == "exp") return exp(x);    // Экспонента
        if (name == "log") {                 // Натуральный логарифм
            if (x < 0) {                     // Отрицательный аргумент
                throw runtime_error("Domain error: log(" + format_double_for_error(x) + ")");
            }
            if (x == 0) {                    // Ноль
                return -INFINITY;            // Логарифм нуля даёт -∞
            }
            return log(x);
        }
        if (name == "sqrt") {                // Квадратный корень
            if (x < 0) {                     // Отрицательный аргумент
                throw runtime_error("Domain error: sqrt(" + format_double_for_error(x) + ")");
            }
            return sqrt(x);
        }
        throw runtime_error("Unknown function: " + name);  // Неизвестная функция
    }

    Node* clone() const override { return new FunctionNode(name, arg->clone()); }  // Клонирование

    Node* diff(const string& var) const override {  // Производная функции
        Node* du = arg->diff(var);        // Производная аргумента u'
        if (name == "sin") {              // sin(u)' = cos(u) * u'
            return new BinaryOpNode('*', new FunctionNode("cos", arg->clone()), du);
        }
        if (name == "cos") {              // cos(u)' = -sin(u) * u'
            return new BinaryOpNode('*', new UnaryOpNode('-', new FunctionNode("sin", arg->clone())), du);
        }
        if (name == "tan") {              // tan(u)' = 1/(cos^2(u)) * u'
            Node* cos_u = new FunctionNode("cos", arg->clone());  // cos(u)
            Node* cos2 = new BinaryOpNode('^', cos_u, new NumberNode(2));  // cos^2(u)
            Node* one_div_cos2 = new BinaryOpNode('/', new NumberNode(1), cos2);  // 1/cos^2(u)
            return new BinaryOpNode('*', one_div_cos2, du);
        }
        if (name == "asin") {             // asin(u)' = 1/sqrt(1-u^2) * u'
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));  // u^2
            Node* sqrt_expr = new FunctionNode("sqrt", new BinaryOpNode('-', one->clone(), u2));  // sqrt(1-u^2)
            Node* inv = new BinaryOpNode('/', new NumberNode(1), sqrt_expr);  // 1/sqrt(1-u^2)
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "acos") {             // acos(u)' = -1/sqrt(1-u^2) * u'
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));
            Node* sqrt_expr = new FunctionNode("sqrt", new BinaryOpNode('-', one->clone(), u2));
            Node* inv = new BinaryOpNode('/', new NumberNode(1), sqrt_expr);
            return new BinaryOpNode('*', new UnaryOpNode('-', inv), du);
        }
        if (name == "atan") {             // atan(u)' = 1/(1+u^2) * u'
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));  // u^2
            Node* denom = new BinaryOpNode('+', one, u2);  // 1+u^2
            Node* inv = new BinaryOpNode('/', new NumberNode(1), denom);  // 1/(1+u^2)
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "exp") {              // exp(u)' = exp(u) * u'
            return new BinaryOpNode('*', new FunctionNode("exp", arg->clone()), du);
        }
        if (name == "log") {              // ln(u)' = 1/u * u'
            Node* inv = new BinaryOpNode('/', new NumberNode(1), arg->clone());  // 1/u
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "sqrt") {             // sqrt(u)' = 1/(2*sqrt(u)) * u'
            Node* two = new NumberNode(2);
            Node* sqrt_arg = new FunctionNode("sqrt", arg->clone());  // sqrt(u)
            Node* denom = new BinaryOpNode('*', two, sqrt_arg);  // 2*sqrt(u)
            Node* inv = new BinaryOpNode('/', new NumberNode(1), denom);  // 1/(2*sqrt(u))
            return new BinaryOpNode('*', inv, du);
        }
        throw runtime_error("Unknown function in derivative: " + name);  // Неизвестная функция
    }

    string to_string() const override {  // Строковое представление: имя(аргумент)
        return name + "(" + arg->to_string() + ")";
    }
};

// ============================================================================
// BinaryOpNode (определение методов)
// ============================================================================

double BinaryOpNode::eval(const map<string, double>& vars) const {  // Вычисление бинарной операции
    double l = left->eval(vars);   // Вычисляем левый операнд
    double r = right->eval(vars);  // Вычисляем правый операнд
    switch (op) {                  // В зависимости от операции
        case '+': return l + r;    // Сложение
        case '-': return l - r;    // Вычитание
        case '*': return l * r;    // Умножение
        case '/':                  // Деление
            if (r == 0.0) return INFINITY;  // Деление на ноль даёт бесконечность
            return l / r;
        case '^':                  // Возведение в степень
            return pow(l, r);
        default:                   // Неизвестный оператор
            throw runtime_error("Unknown binary operator");
    }
}

Node* BinaryOpNode::diff(const string& var) const {  // Дифференцирование бинарной операции
    switch (op) {
        case '+':  // (u+v)' = u' + v'
            return new BinaryOpNode('+', left->diff(var), right->diff(var));
        case '-':  // (u-v)' = u' - v'
            return new BinaryOpNode('-', left->diff(var), right->diff(var));
        case '*':  // (u*v)' = u'*v + u*v'
            return new BinaryOpNode('+',
                new BinaryOpNode('*', left->diff(var), right->clone()),
                new BinaryOpNode('*', left->clone(), right->diff(var))
            );
        case '/':  // (u/v)' = (u'*v - u*v')/v^2
            return new BinaryOpNode('/',
                new BinaryOpNode('-',
                    new BinaryOpNode('*', left->diff(var), right->clone()),
                    new BinaryOpNode('*', left->clone(), right->diff(var))
                ),
                new BinaryOpNode('^', right->clone(), new NumberNode(2))
            );
        case '^': {  // Возведение в степень: требуется отдельная обработка
            if (right->is_constant()) {  // Если показатель степени - константа: (u^n)' = n*u^(n-1)*u'
                NumberNode* rnum = dynamic_cast<NumberNode*>(right);
                double power = rnum->value;  // Значение показателя
                Node* new_power = new NumberNode(power);  // n
                Node* new_exp = new BinaryOpNode('^', left->clone(), new NumberNode(power - 1));  // u^(n-1)
                Node* mul1 = new BinaryOpNode('*', new_power, new_exp);  // n*u^(n-1)
                return new BinaryOpNode('*', mul1, left->diff(var));  // n*u^(n-1)*u'
            } else {  // Общий случай: (u^v)' = u^v * (v'*ln(u) + v*u'/u)
                Node* u = left->clone();       // u
                Node* v = right->clone();      // v
                Node* du = left->diff(var);    // u'
                Node* dv = right->diff(var);   // v'
                Node* ln_u = new FunctionNode("log", u->clone());  // ln(u)
                Node* v_ln_u = new BinaryOpNode('*', v->clone(), ln_u);  // v*ln(u)
                Node* dv_ln_u = new BinaryOpNode('*', dv, ln_u->clone());  // v'*ln(u)
                Node* v_du_div_u = new BinaryOpNode('*', v->clone(),
                    new BinaryOpNode('/', du, u->clone()));  // v*u'/u
                Node* sum = new BinaryOpNode('+', dv_ln_u, v_du_div_u);  // v'*ln(u) + v*u'/u
                Node* u_pow_v = new BinaryOpNode('^', u->clone(), v->clone());  // u^v
                return new BinaryOpNode('*', u_pow_v, sum);  // u^v * (v'*ln(u) + v*u'/u)
            }
        }
        default:
            throw runtime_error("Unknown operator in derivative");
    }
}

// ============================================================================
// Парсер
// ============================================================================

class Parser {  // Класс парсера для построения AST из токенов
public:
    Parser(const vector<Token>& tokens) : tokens(tokens), idx(0) {}  // Конструктор: сохраняем токены, начальный индекс 0

    Node* parse() {                     // Основной метод парсера
        Node* expr = parse_expression();  // Разбираем выражение
        if (current_token().type != TokenType::END) {  // Если после выражения не конец строки
            throw runtime_error("Unexpected tokens after expression");  // Ошибка
        }
        return expr;                    // Возвращаем корневой узел AST
    }

private:
    const vector<Token>& tokens;  // Ссылка на вектор токенов
    size_t idx;                   // Текущий индекс в векторе токенов

    Token current_token() const {  // Получение текущего токена
        if (idx < tokens.size()) return tokens[idx];  // Если индекс в пределах, возвращаем токен
        return Token(TokenType::END, tokens.back().pos);  // Иначе возвращаем END
    }

    void consume() { idx++; }  // Переход к следующему токену

    void expect(TokenType type, const string& err_msg) {  // Проверка ожидаемого типа токена
        if (current_token().type != type) {  // Если тип не совпадает
            throw runtime_error(err_msg);    // Ошибка с сообщением
        }
        consume();  // Переходим к следующему токену
    }

    Node* parse_expression() {  // Разбор выражения (сложения/вычитания)
        Node* node = parse_term();  // Разбираем левую часть (терм)
        while (true) {              // Цикл обработки бинарных операторов + и -
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && (tok.str_value == "+" || tok.str_value == "-")) {
                consume();                 // Поглощаем оператор
                Node* right = parse_term();  // Разбираем правую часть (терм)
                node = new BinaryOpNode(tok.str_value[0], node, right);  // Создаём узел бинарной операции
            } else {
                break;  // Если нет оператора + или -, выходим из цикла
            }
        }
        return node;  // Возвращаем узел выражения
    }

    Node* parse_term() {  // Разбор терма (умножения/деления)
        Node* node = parse_power();  // Разбираем левую часть (степень)
        while (true) {               // Цикл обработки бинарных операторов * и /
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && (tok.str_value == "*" || tok.str_value == "/")) {
                consume();                  // Поглощаем оператор
                Node* right = parse_power();  // Разбираем правую часть (степень)
                node = new BinaryOpNode(tok.str_value[0], node, right);  // Создаём узел бинарной операции
            } else {
                break;  // Если нет оператора * или /, выходим из цикла
            }
        }
        return node;  // Возвращаем узел терма
    }

    Node* parse_power() {  // Разбор степени (правоассоциативная операция ^)
        Node* node = parse_unary();  // Разбираем левую часть (унарное выражение)
        while (true) {               // Цикл обработки оператора ^
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && tok.str_value == "^") {
                consume();                  // Поглощаем оператор
                Node* right = parse_power();  // Рекурсивный разбор правой части (правоассоциативность)
                node = new BinaryOpNode('^', node, right);  // Создаём узел степени
            } else {
                break;  // Если нет оператора ^, выходим из цикла
            }
        }
        return node;  // Возвращаем узел степени
    }

    Node* parse_unary() {  // Разбор унарных операторов + и -
        Token tok = current_token();
        if (tok.type == TokenType::OPERATOR && (tok.str_value == "+" || tok.str_value == "-")) {
            consume();                      // Поглощаем оператор
            Node* operand = parse_power();   // Разбираем операнд (степень)
            if (tok.str_value == "-") {      // Если унарный минус
                return new UnaryOpNode('-', operand);  // Создаём узел унарного минуса
            } else {                         // Если унарный плюс
                return operand;              // Просто возвращаем операнд
            }
        }
        return parse_primary();  // Иначе разбираем первичное выражение
    }

    Node* parse_primary() {  // Разбор первичного выражения (число, переменная, функция, выражение в скобках)
        Token tok = current_token();
        if (tok.type == TokenType::NUMBER) {  // Если число
            consume();                        // Поглощаем токен
            return new NumberNode(tok.num_value);  // Создаём узел числа
        } else if (tok.type == TokenType::IDENTIFIER) {  // Если идентификатор
            string name = to_lower(tok.str_value);  // Приводим к нижнему регистру
            consume();                           // Поглощаем токен
            if (current_token().type == TokenType::LPAREN) {  // Если следующий токен - '(' (вызов функции)
                if (!is_function_name(name)) {    // Проверяем, является ли имя функцией
                    throw runtime_error("Unknown function: " + name);  // Ошибка: неизвестная функция
                }
                consume();                        // Поглощаем '('
                Node* arg = parse_expression();   // Разбираем аргумент функции
                expect(TokenType::RPAREN, "Expected ')' after function argument");  // Ожидаем ')'
                return new FunctionNode(name, arg);  // Создаём узел функции
            } else {                              // Иначе это переменная
                if (is_function_name(name)) {      // Если имя совпадает с именем функции
                    throw runtime_error("Variable name cannot be same as function name: " + name);  // Ошибка
                }
                return new VariableNode(name);    // Создаём узел переменной
            }
        } else if (tok.type == TokenType::LPAREN) {  // Если '('
            consume();                        // Поглощаем '('
            Node* expr = parse_expression();   // Разбираем выражение внутри скобок
            expect(TokenType::RPAREN, "Expected ')'");  // Ожидаем ')'
            return expr;                      // Возвращаем узел выражения
        } else {                              // Иначе ошибка
            throw runtime_error("Unexpected token in primary expression");
        }
    }
};

// ============================================================================
// Режим лексера
// ============================================================================

void run_lexer_mode(const string& input) {  // Функция запуска режима лексера (вывод токенов)
    try {
        Lexer lexer(input);                 // Создаём лексер
        vector<Token> tokens = lexer.tokenize();  // Получаем токены
        for (const auto& tok : tokens) {    // Проходим по всем токенам
            if (tok.type == TokenType::NUMBER) {       // Если число
                cout << tok.raw_value << endl;         // Выводим исходную строку числа
            } else if (tok.type == TokenType::IDENTIFIER) {  // Если идентификатор
                cout << tok.str_value << endl;         // Выводим строку идентификатора
            } else if (tok.type == TokenType::OPERATOR) {    // Если оператор
                cout << tok.str_value << endl;         // Выводим символ оператора
            } else if (tok.type == TokenType::LPAREN) {      // Если '('
                cout << "(" << endl;                   // Выводим '('
            } else if (tok.type == TokenType::RPAREN) {      // Если ')'
                cout << ")" << endl;                   // Выводим ')'
            }
        }
    } catch (const exception& e) {          // В случае ошибки
        cout << "ERROR: " << e.what() << endl;  // Выводим сообщение об ошибке
    }
}

// ============================================================================
// Основная программа
// ============================================================================

int main() {                                // Главная функция программы
    string first_line;                      // Переменная для первой строки ввода
    if (!getline(cin, first_line)) return 0;  // Читаем первую строку, если нет данных - выходим
    if (first_line.empty()) return 0;       // Если строка пустая - выходим

    if (first_line != "evaluate" && first_line != "derivative" && first_line != "evaluate_derivative") {
        // Если первая строка не одна из команд - переключаемся в режим лексера
        string rest;                        // Переменная для остальных строк
        string line;                        // Буфер для чтения строк
        while (getline(cin, line)) {        // Читаем все оставшиеся строки
            if (!rest.empty()) rest += "\n";  // Добавляем перенос строки между строками
            rest += line;                   // Добавляем строку в rest
        }
        string full_input = first_line + (rest.empty() ? "" : "\n" + rest);  // Объединяем всё
        run_lexer_mode(full_input);         // Запускаем режим лексера
        return 0;                           // Выход из программы
    }

    try {                                   // Начало блока обработки ошибок
        string command_line = first_line;   // Сохраняем команду
        string n_line;                      // Переменная для строки с количеством переменных
        if (!getline(cin, n_line)) throw runtime_error("Missing number of variables");  // Читаем количество
        int n = stoi(n_line);               // Преобразуем в целое число

        vector<string> var_names;           // Вектор имён переменных
        while ((int)var_names.size() < n) { // Пока не прочитали все имена
            string line;                    // Строка с именами
            if (!getline(cin, line)) throw runtime_error("Insufficient variable names");  // Читаем строку
            istringstream iss(line);        // Создаём поток для разбора
            string name;                    // Буфер для имени
            while (iss >> name) {           // Читаем имена из строки
                var_names.push_back(name);  // Добавляем в вектор
                if ((int)var_names.size() == n) break;  // Если набрали нужное количество - выходим
            }
        }

        vector<double> var_values;          // Вектор значений переменных
        while ((int)var_values.size() < n) {// Пока не прочитали все значения
            string line;                    // Строка со значениями
            if (!getline(cin, line)) throw runtime_error("Insufficient variable values");  // Читаем строку
            istringstream iss(line);        // Создаём поток для разбора
            double val;                     // Буфер для значения
            while (iss >> val) {            // Читаем значения из строки
                var_values.push_back(val);  // Добавляем в вектор
                if ((int)var_values.size() == n) break;  // Если набрали нужное количество - выходим
            }
        }

        string expr_str;                    // Переменная для строки выражения
        if (!getline(cin, expr_str)) throw runtime_error("Missing expression");  // Читаем выражение

        map<string, double> var_map;        // Ассоциативный массив переменных (имя -> значение)
        for (int i = 0; i < n; ++i) {       // Проходим по всем переменным
            var_map[to_lower(var_names[i])] = var_values[i];  // Заполняем map (имена в нижнем регистре)
        }

        Lexer lexer(expr_str);              // Создаём лексер для выражения
        vector<Token> tokens = lexer.tokenize();  // Получаем токены

        Parser parser(tokens);              // Создаём парсер
        unique_ptr<Node> ast(parser.parse());  // Строим AST и сохраняем в умный указатель

        if (command_line == "evaluate") {   // Если команда "evaluate"
            double result = ast->eval(var_map);  // Вычисляем значение выражения
            print_double(result);            // Выводим результат
            cout << endl;                    // Переводим строку
        } else if (command_line == "derivative") {  // Если команда "derivative"
            if (n == 0) throw runtime_error("No variable to differentiate by");  // Проверяем, есть ли переменные
            string first_var = to_lower(var_names[0]);  // Берём первую переменную для дифференцирования
            unique_ptr<Node> deriv(ast->diff(first_var));  // Вычисляем производную
            cout << deriv->to_string() << endl;  // Выводим производную в виде строки
        } else if (command_line == "evaluate_derivative") {  // Если команда "evaluate_derivative"
            if (n == 0) throw runtime_error("No variable to differentiate by");  // Проверяем, есть ли переменные
            string first_var = to_lower(var_names[0]);  // Берём первую переменную для дифференцирования
            unique_ptr<Node> deriv(ast->diff(first_var));  // Вычисляем производную
            double result = deriv->eval(var_map);  // Вычисляем значение производной
            print_double(result);            // Выводим результат
            cout << endl;                    // Переводим строку
        } else {                             // Если неизвестная команда
            throw runtime_error("Unknown command: " + command_line);
        }
    } catch (const exception& e) {           // Обработка исключений
        cout << "ERROR: " << e.what() << endl;  // Выводим сообщение об ошибке
        return 0;                            // Выход из программы
    }
    return 0;                                // Успешное завершение программы
}