#define __STDC_WANT_LIB_EXT1__ 1
#include "pch.h"
#include "CCalcManager.h"
#include "CalculatorManager.h"
#include "CalculatorResource.h"
#include <codecvt>
#include <locale>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#if DEBUG
#if defined(_WINDOWS_)
#include <Windows.h>
#include <strsafe.h>

#define DBGPRINT(kwszDebugFormatString, ...) _DBGPRINT(__FUNCTIONW__, __LINE__, kwszDebugFormatString, __VA_ARGS__)

VOID _DBGPRINT(LPCWSTR kwszFunction, INT iLineNumber, LPCWSTR kwszDebugFormatString, ...)
{
    INT cbFormatString = 0;
    va_list args;
    PWCHAR wszDebugString = NULL;
    size_t st_Offset = 0;

    va_start(args, kwszDebugFormatString);

    cbFormatString = _scwDBGPRINT(L"[%s:%d] ", kwszFunction, iLineNumber) * sizeof(WCHAR);
    cbFormatString += _vscwDBGPRINT(kwszDebugFormatString, args) * sizeof(WCHAR) + 2;

    /* Depending on the size of the format string, allocate space on the stack or the heap. */
    wszDebugString = (PWCHAR)_malloca(cbFormatString);

    /* Populate the buffer with the contents of the format string. */
    StringCbDBGPRINTW(wszDebugString, cbFormatString, L"[%s:%d] ", kwszFunction, iLineNumber);
    StringCbLengthW(wszDebugString, cbFormatString, &st_Offset);
    StringCbVDBGPRINTW(&wszDebugString[st_Offset / sizeof(WCHAR)], cbFormatString - st_Offset, kwszDebugFormatString, args);

    OutputDebugStringW(wszDebugString);

    _freea(wszDebugString);
    va_end(args);
}
#elif defined(__EMSCRIPTEN__)
#define DBGPRINT(kwszDebugFormatString, ...) printf(kwszDebugFormatString, ##__VA_ARGS__);
#endif
#else
#define DBGPRINT(kwszDebugFormatString, ...)
#endif // DEBUG

using namespace CalculationManager;

class CalcDisplay : public ICalcDisplay
{
private:
    CalculatorManager_CreateParams _params;

public:
    CalcDisplay(CalculatorManager_CreateParams params)
    {
        _params = params;
    }

    // Inherited via ICalcDisplay
    virtual void SetPrimaryDisplay(const std::wstring& pszText, bool isError) override
    {
        DBGPRINT("Native:SetPrimaryDisplay(%ls, %d)\n", pszText.data(), isError);

        _params.SetPrimaryDisplay(_params.CalculatorState, pszText.data(), isError);
    }

    virtual void SetIsInError(bool isInError) override
    {
        DBGPRINT("Native:SetIsInError(%d)\n", isInError);

        _params.SetIsInError(_params.CalculatorState, isInError);
    }

    virtual void SetExpressionDisplay(
        std::shared_ptr<CalculatorVector<std::pair<std::wstring, int>>> const& tokens,
        std::shared_ptr<CalculatorVector<std::shared_ptr<IExpressionCommand>>> const& commands) override
    {
        DBGPRINT("Native:SetExpressionDisplay()\n");

        auto item = std::make_shared<HISTORYITEM>();
        item->historyItemVector.expression = L"";
        item->historyItemVector.result = L"";
        item->historyItemVector.spCommands = commands;
        item->historyItemVector.spTokens = tokens;

        auto pItem = MarshalHistoryItem(item);

        _params.SetExpressionDisplay(_params.CalculatorState, pItem);
    }

    virtual void SetParenthesisNumber(unsigned int count) override
    {
        DBGPRINT("Native:SetParenthesisNumber(%d)\n", count);

        _params.SetParenthesisNumber(_params.CalculatorState, count);
    }

    virtual void OnNoRightParenAdded() override
    {
        DBGPRINT("Native:OnNoRightParenAdded()\n");
        _params.OnNoRightParenAdded(_params.CalculatorState);
    }

    virtual void MaxDigitsReached() override
    {
        DBGPRINT("Native:MaxDigitsReached()\n");
        _params.MaxDigitsReached(_params.CalculatorState);
    }

    virtual void BinaryOperatorReceived() override
    {
        DBGPRINT("Native:BinaryOperatorReceived()\n");
        _params.BinaryOperatorReceived(_params.CalculatorState);
    }

    virtual void OnHistoryItemAdded(unsigned int addedItemIndex) override
    {
        DBGPRINT("Native:OnHistoryItemAdded(%d)\n", addedItemIndex);
        _params.OnHistoryItemAdded(_params.CalculatorState, addedItemIndex);
    }

    virtual void SetMemorizedNumbers(const std::vector<std::wstring>& memorizedNumbers) override
    {
        DBGPRINT("Native:SetMemorizedNumbers(%d)\n", (int)memorizedNumbers.size());

        auto numbers = new const wchar_t* [memorizedNumbers.size()] {};

        for (size_t i = 0; i < memorizedNumbers.size(); i++)
        {
            auto str = memorizedNumbers[i];
            auto pData = new wchar_t[str.size() + 1]{};
            str.copy(pData, str.size(), 0);
            numbers[i] = pData;
        }

        _params.SetMemorizedNumbers(_params.CalculatorState, (unsigned int)memorizedNumbers.size(), numbers);

        for (size_t i = 0; i < memorizedNumbers.size(); i++)
        {
            delete[] numbers[i];
        }

        delete[] numbers;
    }

    virtual void MemoryItemChanged(unsigned int indexOfMemory) override
    {
        DBGPRINT("Native:MemoryItemChanged(%d)\n", indexOfMemory);

        _params.MemoryItemChanged(_params.CalculatorState, indexOfMemory);
    }
};

class ResourceProvider : public CalculationManager::IResourceProvider
{
private:
    CalculatorManager_CreateParams _params;

public:
    ResourceProvider(CalculatorManager_CreateParams params)
    {
        _params = params;
    }

    virtual std::wstring GetCEngineString(const std::wstring& id) override
    {
        auto pResult = _params.GetCEngineString(_params.ResourceState, id.data());
        auto str = std::wstring(pResult);
        DBGPRINT("Native:GetCEngineString(id=%ls, str.data()=%ls)\n", id.data(), str.data());
        return str;
    }
};

CalculatorManager* AsManager(void* manager)
{
    return static_cast<CalculatorManager*>(manager);
}

IExpressionCommand* AsIExpressionCommand(void* pExpressionCommand)
{
    return static_cast<IExpressionCommand*>(pExpressionCommand);
}

COpndCommand* AsCOpndCommand(void* pExpressionCommand)
{
    return static_cast<COpndCommand*>(pExpressionCommand);
}

CUnaryCommand* AsCUnaryCommand(void* pExpressionCommand)
{
    return static_cast<CUnaryCommand*>(pExpressionCommand);
}

CBinaryCommand* AsCBinaryCommand(void* pExpressionCommand)
{
    return static_cast<CBinaryCommand*>(pExpressionCommand);
}

const wchar_t* ToWChar(std::wstring& str)
{
    auto out = new wchar_t[str.size() + 1]{};
    str.copy(out, str.size() + 1, 0);
    return out;
}

GetHistoryItemResult* MarshalHistoryItem(std::shared_ptr<CalculationManager::HISTORYITEM>& historyItem)
{
    auto itemResult = new GetHistoryItemResult{};

    itemResult->expression = ToWChar(historyItem->historyItemVector.expression);
    itemResult->result = ToWChar(historyItem->historyItemVector.result);

    unsigned int tokenCount;
    historyItem->historyItemVector.spTokens->GetSize(&tokenCount);
    itemResult->TokenCount = tokenCount;

    //
    // Marshal Tokens
    //
    auto tokenStrings = new const wchar_t* [tokenCount] {};
    auto tokenValues = new int32_t[tokenCount]{};

    // DBGPRINT(L"TokenCount: %d (int32_t: %d)\n", tokenCount, sizeof(int32_t));

    for (uint32_t j = 0; j < tokenCount; j++)
    {
        std::pair<std::wstring, int> pair;

        if (SUCCEEDED(historyItem->historyItemVector.spTokens->GetAt(j, &pair)))
        {
            tokenStrings[j] = ToWChar(pair.first);
            tokenValues[j] = (int32_t)pair.second;
            // DBGPRINT(L"\tPair: %ws;%d\n", pair.first.data(), tokenValues[j]);
        }
    }

    itemResult->TokenStrings = tokenStrings;
    itemResult->TokenValues = tokenValues;

    //
    // Marshal Commands
    //
    unsigned int commandCount;
    historyItem->historyItemVector.spCommands->GetSize(&commandCount);
    itemResult->CommandCount = commandCount;

    auto commands = new void* [commandCount] {};

    for (uint32_t commandId = 0; commandId < commandCount; commandId++)
    {
        std::shared_ptr<IExpressionCommand> command;
        if (SUCCEEDED(historyItem->historyItemVector.spCommands->GetAt(commandId, &command)))
        {
            commands[commandId] = command.get();
        }
    }

    itemResult->Commands = commands;

    return itemResult;
}

void* MarshalHistoryItems(std::vector<std::shared_ptr<CalculationManager::HISTORYITEM>>& historyItems)
{
    auto result = new GetHistoryItemsResult{};

    result->ItemsCount = (int32_t)historyItems.size();

    auto resultsArray = new GetHistoryItemResult*[result->ItemsCount];
    result->HistoryItems = (void*)resultsArray;

    for (size_t i = 0; i < historyItems.size(); i++)
    {
        auto historyItem = historyItems[i];

        resultsArray[i] = MarshalHistoryItem(historyItem);
    }

    return result;
}

void* CalculatorManager_Create(CalculatorManager_CreateParams* pParams)
{
    auto calcDisplay = new CalcDisplay(*pParams);
    auto resProvider = new ResourceProvider(*pParams);

    auto cm = new CalculatorManager(calcDisplay, resProvider);
    return cm;
}

void CalculatorManager_SendCommand(void* manager, int command)
{
    AsManager(manager)->SendCommand((Command)command);
}

void CalculatorManager_SetRadix(void* manager, RADIX_TYPE iRadixType)
{
    AsManager(manager)->SetRadix(iRadixType);
}

void CalculatorManager_Reset(void* manager, bool clearMemory)
{
    AsManager(manager)->Reset(clearMemory);
}

void CalculatorManager_SetStandardMode(void* manager)
{
    AsManager(manager)->SetStandardMode();
}

void CalculatorManager_SetScientificMode(void* manager)
{
    AsManager(manager)->SetScientificMode();
}

void CalculatorManager_SetProgrammerMode(void* manager)
{
    AsManager(manager)->SetProgrammerMode();
}

void CalculatorManager_MemorizeNumber(void* manager)
{
    AsManager(manager)->MemorizeNumber();
}

void CalculatorManager_MemorizedNumberLoad(void* manager, int value)
{
    AsManager(manager)->MemorizedNumberLoad(value);
}

void CalculatorManager_MemorizedNumberAdd(void* manager, int value)
{
    AsManager(manager)->MemorizedNumberAdd(value);
}

void CalculatorManager_MemorizedNumberSubtract(void* manager, int value)
{
    AsManager(manager)->MemorizedNumberSubtract(value);
}

void CalculatorManager_MemorizedNumberClear(void* manager, int value)
{
    AsManager(manager)->MemorizedNumberClear(value);
}

void CalculatorManager_MemorizedNumberClearAll(void* manager)
{
    AsManager(manager)->MemorizedNumberClearAll();
}

bool CalculatorManager_IsEngineRecording(void* manager)
{
    return AsManager(manager)->IsEngineRecording();
}

void CalculatorManager_SetMemorizedNumbersString(void* manager)
{
    AsManager(manager)->SetMemorizedNumbersString();
}

const wchar_t* CalculatorManager_GetResultForRadix(void* manager, int radix, int precision)
{
    auto res = AsManager(manager)->GetResultForRadix(radix, precision);

    auto out = new wchar_t[res.size() + 1]{};
    res.copy(out, res.size(), 0);

    return out;
}

void CalculatorManager_SetPrecision(void* manager, int precision)
{
    AsManager(manager)->SetPrecision(precision);
}

void CalculatorManager_UpdateMaxIntDigits(void* manager)
{
    AsManager(manager)->UpdateMaxIntDigits();
}

const wchar_t* CalculatorManager_DecimalSeparator(void* manager)
{
    auto res = AsManager(manager)->DecimalSeparator();

    std::wstring ret;
    ret += res;

    return ToWChar(ret);
}

bool CalculatorManager_RemoveHistoryItem(void* manager, int uIdx)
{
    return AsManager(manager)->RemoveHistoryItem(uIdx);
}

void CalculatorManager_ClearHistory(void* manager)
{
    AsManager(manager)->ClearHistory();
}

size_t CalculatorManager_MaxHistorySize(void* manager)
{
    return AsManager(manager)->MaxHistorySize();
}

int CalculatorManager_GetCurrentDegreeMode(void* manager)
{
    return (int)AsManager(manager)->GetCurrentDegreeMode();
}

void CalculatorManager_SetInHistoryItemLoadMode(void* manager, bool isHistoryItemLoadMode)
{
    AsManager(manager)->SetInHistoryItemLoadMode(isHistoryItemLoadMode);
}

void* CalculatorManager_GetHistoryItems(void* manager)
{
    auto historyItems = AsManager(manager)->GetHistoryItems();

    return MarshalHistoryItems(historyItems);
}

void* CalculatorManager_GetHistoryItemsWithMode(void* manager, int mode)
{
    auto historyItems = AsManager(manager)->GetHistoryItems((CALCULATOR_MODE)mode);

    return MarshalHistoryItems(historyItems);
}

void* CalculatorManager_GetHistoryItem(void* manager, int index)
{
    auto historyItem = AsManager(manager)->GetHistoryItem(index);

    return MarshalHistoryItem(historyItem);
}

void Free(void* ptr)
{
    free(ptr);
}

int IExpressionCommand_GetCommandType(void* pExpressionCommand)
{
    return (int)AsIExpressionCommand(pExpressionCommand)->GetCommandType();
}

void* COpndCommand_GetCommands(void* pExpressionCommand)
{
    auto res = AsCOpndCommand(pExpressionCommand)->GetCommands();

    auto pRes = (COpndCommand_GetCommandsResult*)malloc(sizeof(COpndCommand_GetCommandsResult));

    unsigned int commandCount;
    res->GetSize(&commandCount);
    pRes->CommandCount = commandCount;

    auto pCommands = (int32_t*)malloc(commandCount * sizeof(int32_t));
    pRes->pCommands = pCommands;

    for (unsigned int i = 0; i < commandCount; i++)
    {
        int value;
        res->GetAt(i, &value);
        pCommands[i] = (int32_t)value;
    }

    return pRes;
}

void* CUnaryCommand_GetCommands(void* pExpressionCommand)
{
    auto res = AsCUnaryCommand(pExpressionCommand)->GetCommands();

    auto pRes = (CUnaryCommand_GetCommandsResult*)malloc(sizeof(CUnaryCommand_GetCommandsResult));

    unsigned int commandCount;
    res->GetSize(&commandCount);
    pRes->CommandCount = commandCount;

    auto pCommands = (int32_t*)malloc(commandCount * sizeof(int32_t));
    pRes->pCommands = pCommands;

    for (unsigned int i = 0; i < commandCount; i++)
    {
        int value;
        res->GetAt(i, &value);
        pCommands[i] = (int32_t)value;
    }

    return pRes;
}

bool COpndCommand_IsNegative(void* pExpressionCommand)
{
    return (int)AsCOpndCommand(pExpressionCommand)->IsNegative();
}

int CBinaryCommand_GetCommand(void* pExpressionCommand)
{
    return (int)AsCBinaryCommand(pExpressionCommand)->GetCommand();
}
