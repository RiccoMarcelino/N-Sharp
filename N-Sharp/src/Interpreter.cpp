#include "Interpreter.h"
#include "Utils.h"
#include "Internal.h"
#include "Eval.h"

NSharpVariable Interpreter::runFunction(const string& str, vector<NSharpVariable> arguments) {
	if (startsWith(trim(str), "NS.")) {
		NSharpVariable returnValue = NSFunction(str, arguments, *this);

#if PRINT_LOGS
		logInfo("NS Function " + trim(str) + " has been executed, return value: " + anyAsString(returnValue.second));
#endif // PRINT_LOGS

		return returnValue;
	}

	if (contains(str, ".")) {
		vector<string> splitted = split(trim(str), '.');
		string fileName = splitted[0];
		string functionName = splitted[splitted.size() - 1];

		if (otherFiles.find(fileName) != otherFiles.end()) {
			if (otherFiles[fileName].first.find(functionName) != otherFiles[fileName].first.end()) {
				pair<vector<string>, vector<string>> data = otherFiles[fileName].first[functionName];
				vector<string> parameters = data.first;
				vector<string> functionCode = data.second;
				NSharpVariable returnValue = createVariable("NULL", NULL);

				if (arguments.size() < parameters.size() || arguments.size() > parameters.size()) {
					logScriptError("Class function " + str + " does not take " + to_string(arguments.size()) + " arguments", getLine());
				}

				if ((int)arguments.size() > 0) {
					for (int i = 0; i < (int)parameters.size(); i++) {
						localVariables[scopeHeight + 1][parameters[i]] = arguments[i];
					}
				}

				string temp = otherClassName;
				otherClassName = fileName;
				returnValue = start(functionCode);
				otherClassName = temp;

				localVariables[scopeHeight + 1].clear();

#if PRINT_LOGS
				logInfo("Class function " + trim(str) + " has been executed, return value: " + anyAsString(returnValue.second));
#endif // PRINT_LOGS

				return returnValue;
			}
			else {
				logScriptError("Undefined class function: " + functionName, getLine());
			}
		}
		else {
			logScriptError("Undefined class: " + fileName, getLine());
		}
	}

	if (functions.find(trim(str)) != functions.end()) {
		pair<vector<string>, vector<string>> data = functions[trim(str)];
		vector<string> parameters = data.first;
		vector<string> functionCode = data.second;
		NSharpVariable returnValue = createVariable("NULL", NULL);

		if (arguments.size() < parameters.size() || arguments.size() > parameters.size()) {
			logScriptError("Function " + str + " does not take " + to_string(arguments.size()) + " arguments", getLine());
		}

		if ((int)arguments.size() > 0) {
			for (int i = 0; i < (int)parameters.size(); i++) {
				localVariables[scopeHeight + 1][parameters[i]] = arguments[i];
			}
		}

		returnValue = start(functionCode);
		localVariables[scopeHeight + 1].clear();

#if PRINT_LOGS
		logInfo("Function " + trim(str) + " has been executed, return value: " + anyAsString(returnValue.second));
#endif // PRINT_LOGS

		return returnValue;
	}
	else {
		logScriptError("Undefined function error: " + str, getLine());
	}

	return NSharpNULL;
}

NSharpVariable Interpreter::getVariable(const string& name)
{
	if (!otherClassName.empty()) {
		int variableScope = searchLocalVariables(name);
		if (scopeHeight > 0 && variableScope != -1) {
			return localVariables[variableScope][name];
		}

		if (otherFiles[otherClassName].second.find(name) == otherFiles[otherClassName].second.end()) {
			logScriptError("Variable not defined error: " + name, getLine());
		}

		return otherFiles[otherClassName].second[name];
	}

	if (contains(name, ".")) {
		vector<string> splitted = split(trim(name), '.');
		string fileName = splitted[0];
		string varName = splitted[splitted.size() - 1];

		if (otherFiles.find(fileName) != otherFiles.end()) {
			if (otherFiles[fileName].second.find(varName) != otherFiles[fileName].second.end()) {
				return otherFiles[fileName].second[varName];
			}
			else {
				logScriptError("Undefined class variable: " + varName, getLine());
			}
		}
		else {
			logScriptError("Undefined class: " + fileName, getLine());
		}
	}
	else {
		int variableScope = searchLocalVariables(name);
		if (scopeHeight > 0 && variableScope != -1) {
			return localVariables[variableScope][name];
		}

		if (globalVariables.find(name) == globalVariables.end()) {
			logScriptError("Variable not defined error: " + name, getLine());
		}

		return globalVariables[name];
	}

	return NSharpNULL;
}

void Interpreter::setVariable(const string& name, NSharpVariable data, bool forceGlobal)
{
	if (!otherClassName.empty()) {
		if (otherFiles[otherClassName].second.find(name) != otherFiles[otherClassName].second.end()) {
			otherFiles[otherClassName].second[name] = data;
		}
		else if (scopeHeight > 0 && !forceGlobal) {
			localVariables[scopeHeight][name] = data;
		}
		else {
			otherFiles[otherClassName].second[name] = data;
		}
	}
	else {
		if (globalVariables.find(name) != globalVariables.end()) {
			globalVariables[name] = data;
		}
		else if (scopeHeight > 0 && !forceGlobal) {
			localVariables[scopeHeight][name] = data;
		}
		else {
			globalVariables[name] = data;
		}
	}

	if (contains(name, ".")) {
		vector<string> splitted = split(trim(name), '.');
		string fileName = splitted[0];
		string varName = splitted[splitted.size() - 1];

		if (otherFiles.find(fileName) != otherFiles.end()) {
			if (otherFiles[fileName].second.find(varName) != otherFiles[fileName].second.end()) {
				otherFiles[fileName].second[varName] = data;
			}
			else {
				logScriptError("Undefined class variable: " + varName, getLine());
			}
		}
		else {
			logScriptError("Undefined class: " + fileName, getLine());
		}
	}

#if PRINT_LOGS
	logInfo("Variable " + name + " set to " + anyAsString(data.second));
#endif // PRINT_LOGS
}

int Interpreter::searchLocalVariables(const string& name)
{
	if (scopeHeight > 0) {
		for (int i = 1; i <= scopeHeight; i++) {
			if (localVariables[i].find(name) != localVariables[i].end()) {
				return i;
			}
		}
	}

	return -1;
}

bool Interpreter::isFunction(const string& str) {
	return count(str, '(') > 0 && count(str, ')') > 0 && !isNumber(trim(replace(replace(str, ")", ""), "(", ""))) && count(trim(str), '=') == 0;
}

bool Interpreter::isVariableDeclaration(const string& line)
{
	VariableType type = getVariableType(line);
	string str = split(line, '=')[0];

	if (!otherClassName.empty()) {
		string predictedVarName = trim(split(replace(str, " ", ""), '=')[0]);
		return startsWith(trim(str), "var ") || (localVariables[scopeHeight].find(predictedVarName) != localVariables[scopeHeight].end() || otherFiles[otherClassName].second.find(predictedVarName) != otherFiles[otherClassName].second.end());
	}

	if (contains(str, ".")) {
		vector<string> splitted = split(trim(str), '.');

		string fileName = splitted[0];
		string predictedVarName = trim(split(replace(splitted[splitted.size() - 1], " ", ""), '=')[0]);

		if (otherFiles.find(fileName) != otherFiles.end()) {
			if (otherFiles[fileName].second.find(predictedVarName) != otherFiles[fileName].second.end()) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			logScriptError("Undefined class: " + fileName, getLine());
		}
	}
	
	string predictedVarName = trim(split(replace(str, " ", ""), '=')[0]);
	return startsWith(trim(str), "var ") || (localVariables[scopeHeight].find(predictedVarName) != localVariables[scopeHeight].end() || globalVariables.find(predictedVarName) != globalVariables.end());
}

bool Interpreter::isMathExpression(const string& str)
{
	return count(str, '\"') == 0 && (count(str, '+') != 0 || count(str, '-') != 0 || count(str, '*') != 0 || count(str, '/') != 0 || count(str, '^') != 0);
}

bool Interpreter::isUsingTag(const string& str)
{
	return startsWith(trim(str), "using ");
}

bool Interpreter::isFunctionDefinition(const string& str)
{
	return startsWith(trim(str), "function ") && count(str, '(') > 0 && count(str, ')') > 0;
}

bool Interpreter::isIfStatement(const string& str)
{
	return startsWith(trim(str), "if");
}

bool Interpreter::isReturnStatement(const string& str)
{
	return startsWith(trim(str), "return ");
}

bool Interpreter::isWhileStatement(const string& str)
{
	return startsWith(trim(str), "while");
}

string Interpreter::getLine()
{
	return currentLine;
}

NSharpVariable Interpreter::start(vector<string>& code, bool isMain) {
	globalVariables["NULL"] = NSharpNULL;
	NSharpVariable toReturn = NSharpNULL;

	if (!isMain) {
		scopeHeight++;
	}
	
	for (int i = 0; i < (int)code.size(); i++) {
		string line = trim(code[i]);
		currentLine = line;
		toReturn = executeLine(line, code, &i);
	}

	if (!isMain) {
		localVariables[scopeHeight].clear();
		scopeHeight--;
	}

	return toReturn;
}

NSharpVariable Interpreter::executeLine(const string& str, vector<string> code, int* curIndex) {
	if (startsWith(str, "//") || str.empty()) return NSharpNULL;

#if PRINT_LOGS
	logInfo("Executing line: " + trim(str));
#endif // PRINT_LOGS

	if (isUsingTag(str)) {
		string path = parseUsingTag(str);
		vector<string> code = readFromFile(path);
		vector<string> parsed = split(replace(str, "using ", ""), '.');
		string tagName = parsed[parsed.size() - 1];

		if (otherFiles.find(tagName) != otherFiles.end()) {
#if PRINT_LOGS
			logInfo("File already included, skipping...:" + trim(path));
#endif // PRINT_LOGS
		}

#if PRINT_LOGS
		logInfo("Executing using tag, path: " + trim(path));
#endif // PRINT_LOGS

		if (code.empty()) {
			logScriptError("Cannot find " + tagName, getLine());
		}

		Interpreter interpreter;
		interpreter.start(code, true);
		otherFiles[tagName].first = interpreter.functions;
		otherFiles[tagName].second = interpreter.globalVariables;

#if PRINT_LOGS
		logInfo("Completed executing using tag, path: " + trim(path));
#endif // PRINT_LOGS
	}
	else if (isWhileStatement(str)) {
		string statement = parseWhileStatement(str);
		vector<string> statementCode = parseCodeInsideBrackets(code, getLine(), curIndex);

#if PRINT_LOGS
		logInfo("Executing while loop, statement: " + trim(statement));
#endif // PRINT_LOGS

		while (evaluateBoolExpression(statement, *this)) {
			start(statementCode);
		}
	}
	else if (isReturnStatement(str)) {
		string data = parseReturnStatement(str);
		*curIndex = (int)code.size();

#if PRINT_LOGS
		logInfo("Executing return statement, value: " + trim(data));
#endif // PRINT_LOGS

		return getAnyFromParameter(data, *this);
	}
	else if (isIfStatement(str)) {
		string parsed = parseIfStatement(str);
		vector<string> statementCode = parseCodeInsideBrackets(code, getLine(), curIndex);

#if PRINT_LOGS
		logInfo("Executing if statement, statement: " + trim(parsed));
#endif // PRINT_LOGS

		if (evaluateBoolExpression(parsed, *this)) {
			start(statementCode);
		}
	}
	else if (isFunctionDefinition(str)) {
		if (scopeHeight != 0) {
			logScriptError("You cannot define functions inside of if statements or funtions", getLine());
		}

		vector<string> functionCode = parseCodeInsideBrackets(code, getLine(), curIndex);

		string functionName = parseFunctionDefinition(str);
		if (contains(functionName, ".")) {
			logScriptError("Function names cannot contain: .", getLine());
		}

		string s = trim(replace(split(str, '(')[1], ")", ""));
		vector<string> parameters = splitOutsideDoubleQuotes(s, ',');

		for (int i = 0; i < (int)parameters.size(); i++) {
			parameters[i] = trim(parameters[i]);
		}

#if PRINT_LOGS
		logInfo("Declaring function: " + trim(functionName));
#endif // PRINT_LOGS

		functions[functionName] = make_pair(parameters, functionCode);
	}
	else if (isFunction(str)) {
		string functionName = parseFunctionCall(str);
		vector<NSharpVariable> params = getParameters(str, *this);

#if PRINT_LOGS
		logInfo("Executing function: " + trim(functionName));
#endif // PRINT_LOGS

		runFunction(functionName, params);
	}
	else if (isVariableDeclaration(str)) {
		string variableName = parseVariableDeclaration(str);
		NSharpVariable data = getAnyFromParameter(trim(split(str, '=')[1]), *this);
	
		if (startsWith(trim(str), "var ") && (globalVariables.find(variableName) != globalVariables.end() || searchLocalVariables(variableName) != -1)) {
			logScriptError("Variable " + variableName + " is already defined", getLine());
		}

#if PRINT_LOGS
		logInfo("Setting variable: " + trim(variableName));
#endif // PRINT_LOGS

		setVariable(variableName, data);
	}
	else {
		logScriptError("Syntax error", getLine());
	}

	return NSharpNULL;
}