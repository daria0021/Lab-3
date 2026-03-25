#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <memory>
#include <algorithm>
#include <set>
#include <stdexcept>

using namespace std;

// ============================================================================
// Вспомогательные функции
// ============================================================================

string to_lower(const string& s) {
    string res = s;
    transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

// Форматирование числа для вывода в сообщениях об ошибках
string format_double_for_error(double d) {
    ostringstream oss;
    oss << d;
    string s = oss.str();
    if (s.find('.') != string::npos) {
        s.erase(s.find_last_not_of('0') + 1, string::npos);
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

// Вывод числа с удалением лишних нулей
void print_double(double d) {
    if (isinf(d)) {
        if (d > 0)
            cout << "inf";
        else
            cout << "-inf";
        return;
    }
    if (isnan(d)) {
        cout << "nan";
        return;
    }

    // Используем стандартный вывод double
    ostringstream oss;
    oss << d;
    string s = oss.str();

    // Удаляем конечные нули, но оставляем хотя бы один знак после запятой если есть
    if (s.find('.') != string::npos) {
        s.erase(s.find_last_not_of('0') + 1, string::npos);
        if (s.back() == '.') s.pop_back();
    }

    cout << s;
}

bool is_function_name(const string& name) {
    static const set<string> functions = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "exp", "log", "sqrt"
    };
    return functions.count(to_lower(name)) > 0;
}

// ============================================================================
// Токены
// ============================================================================

enum class TokenType {
    NUMBER,
    IDENTIFIER,
    OPERATOR,
    LPAREN,
    RPAREN,
    END
};

struct Token {
    TokenType type;
    double num_value;
    string str_value;      // для IDENTIFIER и OPERATOR
    string raw_value;      // для NUMBER – исходная строка
    size_t pos;

    Token(TokenType t, double val, const string& raw, size_t p)
        : type(t), num_value(val), raw_value(raw), pos(p) {}
    Token(TokenType t, const string& s, size_t p)
        : type(t), str_value(s), pos(p) {}
    Token(TokenType t, size_t p) : type(t), pos(p) {}
};

// ============================================================================
// Лексер
// ============================================================================

class Lexer {
public:
    Lexer(const string& input) : input(input), pos(0) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        while (pos < input.size()) {
            char c = input[pos];
            if (isspace(c)) {
                pos++;
                continue;
            }
            if (isdigit(c)) {
                tokens.push_back(parse_number());
            } else if (isalpha(c) || c == '_') {
                tokens.push_back(parse_identifier());
            } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
                tokens.emplace_back(TokenType::OPERATOR, string(1, c), pos);
                pos++;
            } else if (c == '(') {
                tokens.emplace_back(TokenType::LPAREN, pos);
                pos++;
            } else if (c == ')') {
                tokens.emplace_back(TokenType::RPAREN, pos);
                pos++;
            } else {
                throw runtime_error("Unexpected character: " + string(1, c));
            }
        }
        tokens.emplace_back(TokenType::END, pos);
        return tokens;
    }

private:
    string input;
    size_t pos;

    Token parse_number() {
        size_t start = pos;
        bool leading_zero = false;

        if (input[pos] == '0') {
            leading_zero = true;
        }
        pos++;

        while (pos < input.size() && isdigit(input[pos])) {
            if (leading_zero) {
                throw runtime_error("Leading zeros not allowed in number");
            }
            pos++;
        }

        if (pos < input.size() && input[pos] == '.') {
            pos++;
            if (pos >= input.size() || !isdigit(input[pos])) {
                throw runtime_error("Expected digit after decimal point");
            }
            while (pos < input.size() && isdigit(input[pos])) {
                pos++;
            }
        }

        if (pos < input.size() && (isalpha(input[pos]) || input[pos] == '_')) {
            throw runtime_error("Invalid character in number");
        }

        string num_str = input.substr(start, pos - start);
        double val;
        istringstream iss(num_str);
        iss >> val;
        return Token(TokenType::NUMBER, val, num_str, start);
    }

    Token parse_identifier() {
        size_t start = pos;
        while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
            pos++;
        }
        string ident = input.substr(start, pos - start);
        return Token(TokenType::IDENTIFIER, ident, start);
    }
};

// ============================================================================
// Узлы AST (предварительные объявления)
// ============================================================================

class Node {
public:
    virtual ~Node() = default;
    virtual double eval(const map<string, double>& vars) const = 0;
    virtual Node* clone() const = 0;
    virtual Node* diff(const string& var) const = 0;
    virtual string to_string() const = 0;
    virtual bool is_constant() const { return false; }
};

class NumberNode;
class VariableNode;
class UnaryOpNode;
class BinaryOpNode;
class FunctionNode;

// ============================================================================
// NumberNode
// ============================================================================

class NumberNode : public Node {
public:
    double value;

    NumberNode(double val) : value(val) {}

    double eval(const map<string, double>&) const override { return value; }
    Node* clone() const override { return new NumberNode(value); }
    Node* diff(const string&) const override { return new NumberNode(0); }
    string to_string() const override {
        ostringstream oss;
        oss << value;
        string s = oss.str();
        if (s.find('.') != string::npos) {
            s.erase(s.find_last_not_of('0') + 1, string::npos);
            if (s.back() == '.') s.pop_back();
        }
        return s;
    }
    bool is_constant() const override { return true; }
};

// ============================================================================
// VariableNode
// ============================================================================

class VariableNode : public Node {
public:
    string name;

    VariableNode(const string& n) : name(to_lower(n)) {}

    double eval(const map<string, double>& vars) const override {
        auto it = vars.find(name);
        if (it == vars.end()) {
            throw runtime_error("Unknown variable: " + name);
        }
        return it->second;
    }

    Node* clone() const override { return new VariableNode(name); }
    Node* diff(const string& var) const override {
        return new NumberNode(name == to_lower(var) ? 1.0 : 0.0);
    }
    string to_string() const override { return name; }
    bool is_constant() const override { return false; }
};

// ============================================================================
// UnaryOpNode
// ============================================================================

class UnaryOpNode : public Node {
public:
    char op;
    Node* child;

    UnaryOpNode(char op, Node* child) : op(op), child(child) {}
    ~UnaryOpNode() { delete child; }

    double eval(const map<string, double>& vars) const override {
        double val = child->eval(vars);
        if (op == '-') return -val;
        return val;
    }

    Node* clone() const override { return new UnaryOpNode(op, child->clone()); }

    Node* diff(const string& var) const override {
        if (op == '-') {
            return new UnaryOpNode('-', child->diff(var));
        } else {
            return child->diff(var);
        }
    }

    string to_string() const override {
        if (op == '-') {
            if (dynamic_cast<NumberNode*>(child) || dynamic_cast<VariableNode*>(child)) {
                return "-" + child->to_string();
            } else {
                return "-(" + child->to_string() + ")";
            }
        } else {
            return child->to_string();
        }
    }
};

// ============================================================================
// BinaryOpNode (объявлен, определён после FunctionNode)
// ============================================================================

class BinaryOpNode : public Node {
public:
    char op;
    Node* left;
    Node* right;

    BinaryOpNode(char op, Node* left, Node* right) : op(op), left(left), right(right) {}
    ~BinaryOpNode() { delete left; delete right; }

    double eval(const map<string, double>& vars) const override;
    Node* clone() const override { return new BinaryOpNode(op, left->clone(), right->clone()); }
    Node* diff(const string& var) const override;
    string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
    bool is_constant() const override { return left->is_constant() && right->is_constant(); }
};

// ============================================================================
// FunctionNode
// ============================================================================

class FunctionNode : public Node {
public:
    string name;
    Node* arg;

    FunctionNode(const string& n, Node* a) : name(to_lower(n)), arg(a) {}
    ~FunctionNode() { delete arg; }

    double eval(const map<string, double>& vars) const override {
        double x = arg->eval(vars);
        if (name == "sin") return sin(x);
        if (name == "cos") return cos(x);
        if (name == "tan") return tan(x);
        if (name == "asin") {
            if (x < -1 || x > 1) {
                throw runtime_error("Domain error: asin(" + format_double_for_error(x) + ")");
            }
            return asin(x);
        }
        if (name == "acos") {
            if (x < -1 || x > 1) {
                throw runtime_error("Domain error: acos(" + format_double_for_error(x) + ")");
            }
            return acos(x);
        }
        if (name == "atan") return atan(x);
        if (name == "exp") return exp(x);
        if (name == "log") {
            if (x < 0) {
                throw runtime_error("Domain error: log(" + format_double_for_error(x) + ")");
            }
            if (x == 0) {
                return -INFINITY;
            }
            return log(x);
        }
        if (name == "sqrt") {
            if (x < 0) {
                throw runtime_error("Domain error: sqrt(" + format_double_for_error(x) + ")");
            }
            return sqrt(x);
        }
        throw runtime_error("Unknown function: " + name);
    }

    Node* clone() const override { return new FunctionNode(name, arg->clone()); }

    Node* diff(const string& var) const override {
        Node* du = arg->diff(var);
        if (name == "sin") {
            return new BinaryOpNode('*', new FunctionNode("cos", arg->clone()), du);
        }
        if (name == "cos") {
            return new BinaryOpNode('*', new UnaryOpNode('-', new FunctionNode("sin", arg->clone())), du);
        }
        if (name == "tan") {
            Node* cos_u = new FunctionNode("cos", arg->clone());
            Node* cos2 = new BinaryOpNode('^', cos_u, new NumberNode(2));
            Node* one_div_cos2 = new BinaryOpNode('/', new NumberNode(1), cos2);
            return new BinaryOpNode('*', one_div_cos2, du);
        }
        if (name == "asin") {
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));
            Node* sqrt_expr = new FunctionNode("sqrt", new BinaryOpNode('-', one->clone(), u2));
            Node* inv = new BinaryOpNode('/', new NumberNode(1), sqrt_expr);
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "acos") {
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));
            Node* sqrt_expr = new FunctionNode("sqrt", new BinaryOpNode('-', one->clone(), u2));
            Node* inv = new BinaryOpNode('/', new NumberNode(1), sqrt_expr);
            return new BinaryOpNode('*', new UnaryOpNode('-', inv), du);
        }
        if (name == "atan") {
            Node* one = new NumberNode(1);
            Node* u2 = new BinaryOpNode('^', arg->clone(), new NumberNode(2));
            Node* denom = new BinaryOpNode('+', one, u2);
            Node* inv = new BinaryOpNode('/', new NumberNode(1), denom);
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "exp") {
            return new BinaryOpNode('*', new FunctionNode("exp", arg->clone()), du);
        }
        if (name == "log") {
            Node* inv = new BinaryOpNode('/', new NumberNode(1), arg->clone());
            return new BinaryOpNode('*', inv, du);
        }
        if (name == "sqrt") {
            Node* two = new NumberNode(2);
            Node* sqrt_arg = new FunctionNode("sqrt", arg->clone());
            Node* denom = new BinaryOpNode('*', two, sqrt_arg);
            Node* inv = new BinaryOpNode('/', new NumberNode(1), denom);
            return new BinaryOpNode('*', inv, du);
        }
        throw runtime_error("Unknown function in derivative: " + name);
    }

    string to_string() const override {
        return name + "(" + arg->to_string() + ")";
    }
};

// ============================================================================
// BinaryOpNode (определение методов)
// ============================================================================

double BinaryOpNode::eval(const map<string, double>& vars) const {
    double l = left->eval(vars);
    double r = right->eval(vars);
    switch (op) {
        case '+': return l + r;
        case '-': return l - r;
        case '*': return l * r;
        case '/':
            if (r == 0.0) return INFINITY;
            return l / r;
        case '^':
            return pow(l, r);
        default:
            throw runtime_error("Unknown binary operator");
    }
}

Node* BinaryOpNode::diff(const string& var) const {
    switch (op) {
        case '+':
            return new BinaryOpNode('+', left->diff(var), right->diff(var));
        case '-':
            return new BinaryOpNode('-', left->diff(var), right->diff(var));
        case '*':
            return new BinaryOpNode('+',
                new BinaryOpNode('*', left->diff(var), right->clone()),
                new BinaryOpNode('*', left->clone(), right->diff(var))
            );
        case '/':
            return new BinaryOpNode('/',
                new BinaryOpNode('-',
                    new BinaryOpNode('*', left->diff(var), right->clone()),
                    new BinaryOpNode('*', left->clone(), right->diff(var))
                ),
                new BinaryOpNode('^', right->clone(), new NumberNode(2))
            );
        case '^': {
            if (right->is_constant()) {
                NumberNode* rnum = dynamic_cast<NumberNode*>(right);
                double power = rnum->value;
                Node* new_power = new NumberNode(power);
                Node* new_exp = new BinaryOpNode('^', left->clone(), new NumberNode(power - 1));
                Node* mul1 = new BinaryOpNode('*', new_power, new_exp);
                return new BinaryOpNode('*', mul1, left->diff(var));
            } else {
                Node* u = left->clone();
                Node* v = right->clone();
                Node* du = left->diff(var);
                Node* dv = right->diff(var);
                Node* ln_u = new FunctionNode("log", u->clone());
                Node* v_ln_u = new BinaryOpNode('*', v->clone(), ln_u);
                Node* dv_ln_u = new BinaryOpNode('*', dv, ln_u->clone());
                Node* v_du_div_u = new BinaryOpNode('*', v->clone(),
                    new BinaryOpNode('/', du, u->clone()));
                Node* sum = new BinaryOpNode('+', dv_ln_u, v_du_div_u);
                Node* u_pow_v = new BinaryOpNode('^', u->clone(), v->clone());
                return new BinaryOpNode('*', u_pow_v, sum);
            }
        }
        default:
            throw runtime_error("Unknown operator in derivative");
    }
}

// ============================================================================
// Парсер
// ============================================================================

class Parser {
public:
    Parser(const vector<Token>& tokens) : tokens(tokens), idx(0) {}

    Node* parse() {
        Node* expr = parse_expression();
        if (current_token().type != TokenType::END) {
            throw runtime_error("Unexpected tokens after expression");
        }
        return expr;
    }

private:
    const vector<Token>& tokens;
    size_t idx;

    Token current_token() const {
        if (idx < tokens.size()) return tokens[idx];
        return Token(TokenType::END, tokens.back().pos);
    }

    void consume() { idx++; }

    void expect(TokenType type, const string& err_msg) {
        if (current_token().type != type) {
            throw runtime_error(err_msg);
        }
        consume();
    }

    Node* parse_expression() {
        Node* node = parse_term();
        while (true) {
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && (tok.str_value == "+" || tok.str_value == "-")) {
                consume();
                Node* right = parse_term();
                node = new BinaryOpNode(tok.str_value[0], node, right);
            } else {
                break;
            }
        }
        return node;
    }

    Node* parse_term() {
        Node* node = parse_power();
        while (true) {
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && (tok.str_value == "*" || tok.str_value == "/")) {
                consume();
                Node* right = parse_power();
                node = new BinaryOpNode(tok.str_value[0], node, right);
            } else {
                break;
            }
        }
        return node;
    }

    Node* parse_power() {
        Node* node = parse_unary();
        while (true) {
            Token tok = current_token();
            if (tok.type == TokenType::OPERATOR && tok.str_value == "^") {
                consume();
                Node* right = parse_power();
                node = new BinaryOpNode('^', node, right);
            } else {
                break;
            }
        }
        return node;
    }

    Node* parse_unary() {
        Token tok = current_token();
        if (tok.type == TokenType::OPERATOR && (tok.str_value == "+" || tok.str_value == "-")) {
            consume();
            Node* operand = parse_power();
            if (tok.str_value == "-") {
                return new UnaryOpNode('-', operand);
            } else {
                return operand;
            }
        }
        return parse_primary();
    }

    Node* parse_primary() {
        Token tok = current_token();
        if (tok.type == TokenType::NUMBER) {
            consume();
            return new NumberNode(tok.num_value);
        } else if (tok.type == TokenType::IDENTIFIER) {
            string name = to_lower(tok.str_value);
            consume();
            if (current_token().type == TokenType::LPAREN) {
                if (!is_function_name(name)) {
                    throw runtime_error("Unknown function: " + name);
                }
                consume();
                Node* arg = parse_expression();
                expect(TokenType::RPAREN, "Expected ')' after function argument");
                return new FunctionNode(name, arg);
            } else {
                if (is_function_name(name)) {
                    throw runtime_error("Variable name cannot be same as function name: " + name);
                }
                return new VariableNode(name);
            }
        } else if (tok.type == TokenType::LPAREN) {
            consume();
            Node* expr = parse_expression();
            expect(TokenType::RPAREN, "Expected ')'");
            return expr;
        } else {
            throw runtime_error("Unexpected token in primary expression");
        }
    }
};

// ============================================================================
// Режим лексера
// ============================================================================

void run_lexer_mode(const string& input) {
    try {
        Lexer lexer(input);
        vector<Token> tokens = lexer.tokenize();
        for (const auto& tok : tokens) {
            if (tok.type == TokenType::NUMBER) {
                cout << tok.raw_value << endl;
            } else if (tok.type == TokenType::IDENTIFIER) {
                cout << tok.str_value << endl;
            } else if (tok.type == TokenType::OPERATOR) {
                cout << tok.str_value << endl;
            } else if (tok.type == TokenType::LPAREN) {
                cout << "(" << endl;
            } else if (tok.type == TokenType::RPAREN) {
                cout << ")" << endl;
            }
        }
    } catch (const exception& e) {
        cout << "ERROR: " << e.what() << endl;
    }
}

// ============================================================================
// Основная программа
// ============================================================================

int main() {
    string first_line;
    if (!getline(cin, first_line)) return 0;
    if (first_line.empty()) return 0;

    if (first_line != "evaluate" && first_line != "derivative" && first_line != "evaluate_derivative") {
        string rest;
        string line;
        while (getline(cin, line)) {
            if (!rest.empty()) rest += "\n";
            rest += line;
        }
        string full_input = first_line + (rest.empty() ? "" : "\n" + rest);
        run_lexer_mode(full_input);
        return 0;
    }

    try {
        string command_line = first_line;
        string n_line;
        if (!getline(cin, n_line)) throw runtime_error("Missing number of variables");
        int n = stoi(n_line);

        vector<string> var_names;
        while ((int)var_names.size() < n) {
            string line;
            if (!getline(cin, line)) throw runtime_error("Insufficient variable names");
            istringstream iss(line);
            string name;
            while (iss >> name) {
                var_names.push_back(name);
                if ((int)var_names.size() == n) break;
            }
        }

        vector<double> var_values;
        while ((int)var_values.size() < n) {
            string line;
            if (!getline(cin, line)) throw runtime_error("Insufficient variable values");
            istringstream iss(line);
            double val;
            while (iss >> val) {
                var_values.push_back(val);
                if ((int)var_values.size() == n) break;
            }
        }

        string expr_str;
        if (!getline(cin, expr_str)) throw runtime_error("Missing expression");

        map<string, double> var_map;
        for (int i = 0; i < n; ++i) {
            var_map[to_lower(var_names[i])] = var_values[i];
        }

        Lexer lexer(expr_str);
        vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        unique_ptr<Node> ast(parser.parse());

        if (command_line == "evaluate") {
            double result = ast->eval(var_map);
            print_double(result);
            cout << endl;
        } else if (command_line == "derivative") {
            if (n == 0) throw runtime_error("No variable to differentiate by");
            string first_var = to_lower(var_names[0]);
            unique_ptr<Node> deriv(ast->diff(first_var));
            cout << deriv->to_string() << endl;
        } else if (command_line == "evaluate_derivative") {
            if (n == 0) throw runtime_error("No variable to differentiate by");
            string first_var = to_lower(var_names[0]);
            unique_ptr<Node> deriv(ast->diff(first_var));
            double result = deriv->eval(var_map);
            print_double(result);
            cout << endl;
        } else {
            throw runtime_error("Unknown command: " + command_line);
        }
    } catch (const exception& e) {
        cout << "ERROR: " << e.what() << endl;
        return 0;
    }
    return 0;
}