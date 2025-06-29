#define UNICODE
#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mmsystem.h>

#pragma comment(lib, "Winmm.lib")

// Идентификаторы и HWND (без изменений)
// ... (все ID остаются) ...
#define IDC_USERNAME_LABEL 101
#define IDC_USERNAME_EDIT 102
#define IDC_CARD_LABEL 103
#define IDC_CARD_EDIT 104
#define IDC_CVV_LABEL 105
#define IDC_CVV_EDIT 106
#define IDC_AMOUNT_LABEL 107
#define IDC_AMOUNT_EDIT 108
#define IDC_DEPOSIT_BUTTON 109

#define IDC_WELCOME_LABEL 201
#define IDC_BALANCE_LABEL 202
#define IDC_SLOTS_BUTTON 203
#define IDC_ROULETTE_BUTTON 204
#define IDC_CASHOUT_BUTTON 205
#define IDC_REPLENISH_AGAIN_BUTTON 206

#define IDC_REFILL_INFO_LABEL 300
#define IDC_REFILL_AMOUNT_LABEL 301
#define IDC_REFILL_AMOUNT_EDIT 302
#define IDC_REFILL_CONFIRM_BUTTON 303
#define IDC_REFILL_USE_NEW_DETAILS_BUTTON 304
#define IDC_REFILL_CANCEL_BUTTON 305

#define IDC_SLOTS_REEL1_LABEL 401
#define IDC_SLOTS_REEL2_LABEL 402
#define IDC_SLOTS_REEL3_LABEL 403
#define IDC_SLOTS_BET_PROMPT_LABEL 404
#define IDC_SLOTS_BET_EDIT 405
#define IDC_SLOTS_SPIN_BUTTON 406
#define IDC_SLOTS_WIN_INFO_LABEL 407
#define IDC_SLOTS_BACK_BUTTON 408
#define IDC_SLOTS_BALANCE_LABEL 409


// Глобальные HWND (остаются такими же)
// ... (все HWND остаются) ...
HWND hUsernameLabel, hUsernameEdit;
HWND hCardLabel, hCardEdit;
HWND hCvvLabel, hCvvEdit;
HWND hAmountLabel, hAmountEdit;
HWND hDepositButton;

HWND hWelcomeLabel, hBalanceLabel;
HWND hSlotsButton, hRouletteButton, hCashoutButton, hReplenishAgainButton;

HWND hRefillInfoLabel, hRefillAmountLabel, hRefillAmountEdit, hRefillConfirmButton, hRefillUseNewDetailsButton, hRefillCancelButton;

HWND hSlotsReel1, hSlotsReel2, hSlotsReel3;
HWND hSlotsBetPromptLabel, hSlotsBetEdit;
HWND hSlotsSpinButton;
HWND hSlotsWinInfoLabel;
HWND hSlotsBackButton;
HWND hSlotsBalanceLabel;


// Данные пользователя (остаются такими же)
std::wstring g_userName;
std::wstring g_cardNumber;
std::wstring g_cardCVV;
double g_balance = 0.0;

// AppStage (остается таким же)
enum class AppStage { /* ... */
    INITIAL_DEPOSIT,
    GAME_SELECTION,
    REFILL_ENTER_AMOUNT,
    SLOTS_GAME
};
AppStage currentStage = AppStage::INITIAL_DEPOSIT;

// SlotSymbol и slotSymbols (остаются такими же)
struct SlotSymbol { wchar_t visual; int id; };
std::vector<SlotSymbol> slotSymbols = {
    {L'🍒', 0}, {L'🍋', 1}, {L'🍊', 2}, {L'🍉', 3}, {L'🔔', 4}, {L'⭐', 5}, {L'❼', 6}
};
std::vector<SlotSymbol> currentReels(3);

// Имена звуковых файлов (остаются такими же)
const wchar_t* SOUND_WIN_FILE = L"win.wav";
const wchar_t* SOUND_CASHOUT_FILE = L"cashout.wav";

// Прототипы функций
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// ... (остальные прототипы) ...
void CreateStageInitialDepositControls(HWND hwnd, bool fillUsernameAndLock);
void DestroyStageInitialDepositControls();
void CreateStageGameSelectionControls(HWND hwnd);
void DestroyStageGameSelectionControls();
void CreateStageRefillEnterAmountControls(HWND hwnd);
void DestroyStageRefillEnterAmountControls();
void CreateStageSlotsGameControls(HWND hwnd);
void DestroyStageSlotsGameControls();
void SaveUserDataToFile(double lastDepositAmount);
void UpdateBalanceLabelText(HWND parentHwnd, HWND specificLabel = NULL);
bool IsNumeric(const std::wstring& s);
void SpinSlots(HWND hwnd);
void InvalidateControlOnParent(HWND hCtrl, HWND hParent); // НОВАЯ функция

// Вспомогательная функция для инвалидации области контрола на родительском окне
void InvalidateControlOnParent(HWND hCtrl, HWND hParent) {
    if (!hCtrl || !hParent || !IsWindow(hCtrl) || !IsWindow(hParent)) {
        return;
    }
    RECT rcCtrl;
    GetWindowRect(hCtrl, &rcCtrl); // Получаем экранные координаты контрола
    // Конвертируем экранные координаты в клиентские координаты родителя
    MapWindowPoints(HWND_DESKTOP, hParent, (LPPOINT)&rcCtrl, 2);
    InvalidateRect(hParent, &rcCtrl, TRUE); // Инвалидируем эту область на родителе
    // UpdateWindow(hParent); // Можно раскомментировать для немедленной перерисовки родителя в этой области
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // ... (без изменений) ...
    srand(static_cast<unsigned int>(time(0)));

    const wchar_t CLASS_NAME[] = L"GenshinImpactCasinoClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) { /*...*/ return 0; }
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Казино 'Геншин Импакт'",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) { /*...*/ return 0; }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateStageInitialDepositControls(hwnd, false);
        currentStage = AppStage::INITIAL_DEPOSIT;
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        // ... (весь код WM_COMMAND остается таким же, как в предыдущей версии,
        // включая PlaySound и логику переходов между этапами.
        // Я его сокращу здесь для краткости, но он должен быть полным)
        if (currentStage == AppStage::INITIAL_DEPOSIT && wmId == IDC_DEPOSIT_BUTTON) {
            wchar_t usernameBuffer[256], cardBuffer[50], cvvBuffer[10], amountBuffer[50]; GetWindowText(hUsernameEdit, usernameBuffer, 256); GetWindowText(hCardEdit, cardBuffer, 50); GetWindowText(hCvvEdit, cvvBuffer, 10); GetWindowText(hAmountEdit, amountBuffer, 50); std::wstring tempUserNameStr = usernameBuffer; std::wstring tempCardNumberStr = cardBuffer; std::wstring tempCvvStr = cvvBuffer; std::wstring amountStr = amountBuffer; if (IsWindowEnabled(hUsernameEdit)) { if (tempUserNameStr.empty()) { MessageBox(hwnd, L"Введите имя.", L"Ошибка", MB_OK); SetFocus(hUsernameEdit); return 0; } g_userName = tempUserNameStr; } if (tempCardNumberStr.empty() || !IsNumeric(tempCardNumberStr)) { MessageBox(hwnd, L"Номер карты: только цифры.", L"Ошибка", MB_OK); SetFocus(hCardEdit); return 0; } if (tempCvvStr.empty() || !IsNumeric(tempCvvStr) || (tempCvvStr.length() != 3 && tempCvvStr.length() != 4)) { MessageBox(hwnd, L"CVV: 3-4 цифры.", L"Ошибка", MB_OK); SetFocus(hCvvEdit); return 0; } if (amountStr.empty()) { MessageBox(hwnd, L"Введите сумму.", L"Ошибка", MB_OK); SetFocus(hAmountEdit); return 0; } double depositAmount; try { depositAmount = std::stod(amountStr); if (depositAmount <= 0) { MessageBox(hwnd, L"Сумма > 0.", L"Ошибка", MB_OK); SetFocus(hAmountEdit); return 0; } }
            catch (const std::exception&) { MessageBox(hwnd, L"Неверный формат суммы.", L"Ошибка", MB_OK); SetFocus(hAmountEdit); return 0; } g_cardNumber = tempCardNumberStr; g_cardCVV = tempCvvStr; g_balance += depositAmount; SaveUserDataToFile(depositAmount); DestroyStageInitialDepositControls(); CreateStageGameSelectionControls(hwnd); currentStage = AppStage::GAME_SELECTION;
        }
        else if (currentStage == AppStage::GAME_SELECTION) {
            switch (wmId) {
            case IDC_SLOTS_BUTTON: DestroyStageGameSelectionControls(); CreateStageSlotsGameControls(hwnd); currentStage = AppStage::SLOTS_GAME; break;
            case IDC_ROULETTE_BUTTON: MessageBox(hwnd, L"Рулетка пока в разработке!", L"Скоро", MB_OK); break;
            case IDC_CASHOUT_BUTTON: { PlaySound(SOUND_CASHOUT_FILE, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT); wchar_t balanceStr[50]; swprintf(balanceStr, 50, L"%.2f", g_balance); MessageBox(hwnd, (L"Пользователь " + g_userName + L", езжай копать картошку! Твой баланс был " + balanceStr).c_str(), L"Вывод средств", MB_OK | MB_ICONINFORMATION); g_balance = 0; UpdateBalanceLabelText(hwnd, hBalanceLabel); } break;
            case IDC_REPLENISH_AGAIN_BUTTON: { int msgBoxID = MessageBox(hwnd, (L"Использовать сохраненные реквизиты?\nКарта: ****" + (g_cardNumber.length() > 4 ? g_cardNumber.substr(g_cardNumber.length() - 4) : L"XXXX") + L", CVV: ***").c_str(), L"Пополнить баланс", MB_YESNOCANCEL | MB_ICONQUESTION); if (msgBoxID == IDYES) { DestroyStageGameSelectionControls(); CreateStageRefillEnterAmountControls(hwnd); currentStage = AppStage::REFILL_ENTER_AMOUNT; } else if (msgBoxID == IDNO) { DestroyStageGameSelectionControls(); CreateStageInitialDepositControls(hwnd, true); currentStage = AppStage::INITIAL_DEPOSIT; } } break;
            }
        }
        else if (currentStage == AppStage::REFILL_ENTER_AMOUNT) {
            if (wmId == IDC_REFILL_CONFIRM_BUTTON) {
                wchar_t amountBuffer[50]; GetWindowText(hRefillAmountEdit, amountBuffer, 50); std::wstring amountStr = amountBuffer; if (amountStr.empty()) { MessageBox(hwnd, L"Введите сумму.", L"Ошибка", MB_OK); SetFocus(hRefillAmountEdit); return 0; } double depositAmount; try { depositAmount = std::stod(amountStr); if (depositAmount <= 0) { MessageBox(hwnd, L"Сумма > 0.", L"Ошибка", MB_OK); SetFocus(hRefillAmountEdit); return 0; } }
                catch (const std::exception&) { MessageBox(hwnd, L"Неверный формат суммы.", L"Ошибка", MB_OK); SetFocus(hRefillAmountEdit); return 0; } g_balance += depositAmount; SaveUserDataToFile(depositAmount); DestroyStageRefillEnterAmountControls(); CreateStageGameSelectionControls(hwnd); currentStage = AppStage::GAME_SELECTION;
            }
            else if (wmId == IDC_REFILL_USE_NEW_DETAILS_BUTTON) {
                DestroyStageRefillEnterAmountControls(); CreateStageInitialDepositControls(hwnd, true); currentStage = AppStage::INITIAL_DEPOSIT;
            }
            else if (wmId == IDC_REFILL_CANCEL_BUTTON) { DestroyStageRefillEnterAmountControls(); CreateStageGameSelectionControls(hwnd); currentStage = AppStage::GAME_SELECTION; }
        }
        else if (currentStage == AppStage::SLOTS_GAME) {
            if (wmId == IDC_SLOTS_SPIN_BUTTON) {
                SpinSlots(hwnd);
            }
            else if (wmId == IDC_SLOTS_BACK_BUTTON) { DestroyStageSlotsGameControls(); CreateStageGameSelectionControls(hwnd); currentStage = AppStage::GAME_SELECTION; }
        }
    }
                   break;

    case WM_DESTROY:
        // ... (все DestroyWindow...)
        DestroyStageInitialDepositControls(); DestroyStageGameSelectionControls(); DestroyStageRefillEnterAmountControls(); DestroyStageSlotsGameControls();
        PostQuitMessage(0);
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void UpdateBalanceLabelText(HWND parentHwnd, HWND specificLabel) { // parentHwnd здесь - это главное окно
    HWND labelToUpdate = specificLabel;
    if (!labelToUpdate) {
        if (currentStage == AppStage::GAME_SELECTION && hBalanceLabel) labelToUpdate = hBalanceLabel;
        else if (currentStage == AppStage::SLOTS_GAME && hSlotsBalanceLabel) labelToUpdate = hSlotsBalanceLabel;
    }

    if (labelToUpdate && IsWindow(labelToUpdate)) {
        InvalidateControlOnParent(labelToUpdate, parentHwnd); // ИНВАЛИДАЦИЯ НА РОДИТЕЛЕ

        wchar_t balanceStr[50];
        swprintf(balanceStr, 50, L"%.2f", g_balance);
        std::wstring displayText = L"Баланс: " + std::wstring(balanceStr);
        SetWindowText(labelToUpdate, displayText.c_str());
    }
}

void SpinSlots(HWND hwnd) { // hwnd здесь - это главное окно
    // ... (код получения ставки и проверки баланса - без изменений) ...
    wchar_t betAmountStr[50]; GetWindowText(hSlotsBetEdit, betAmountStr, 50); double betAmount; try { betAmount = std::stod(betAmountStr); }
    catch (const std::exception&) { MessageBox(hwnd, L"Неверная сумма ставки.", L"Ошибка", MB_OK | MB_ICONWARNING); SetFocus(hSlotsBetEdit); return; } if (betAmount <= 0) { MessageBox(hwnd, L"Ставка должна быть положительной.", L"Ошибка", MB_OK | MB_ICONWARNING); SetFocus(hSlotsBetEdit); return; } if (g_balance < betAmount) { MessageBox(hwnd, L"Недостаточно средств для ставки!", L"Нет денег", MB_OK | MB_ICONWARNING); SetFocus(hSlotsBetEdit); return; }
    g_balance -= betAmount;
    UpdateBalanceLabelText(hwnd, hSlotsBalanceLabel); // Обновляем баланс до спина

    // Крутим барабаны (этот код инвалидации для барабанов можно оставить, т.к. у них рамка)
    for (int i = 0; i < 3; ++i) { currentReels[i] = slotSymbols[rand() % slotSymbols.size()]; }
    InvalidateRect(hwnd, NULL, TRUE); SetWindowText(hSlotsReel1, (std::wstring(L"") + currentReels[0].visual + L"").c_str()); UpdateWindow(hSlotsReel1);
    InvalidateRect(hwnd, NULL, TRUE); SetWindowText(hSlotsReel2, (std::wstring(L"") + currentReels[1].visual + L"").c_str()); UpdateWindow(hSlotsReel2);
    InvalidateRect(hwnd, NULL, TRUE); SetWindowText(hSlotsReel3, (std::wstring(L"") + currentReels[2].visual + L"").c_str()); UpdateWindow(hSlotsReel3);

    // ... (код проверки выигрыша - без изменений) ...
    double winMultiplier = 0; std::wstring winMessage = L"Попробуйте еще!"; wchar_t s1 = currentReels[0].visual; wchar_t s2 = currentReels[1].visual; wchar_t s3 = currentReels[2].visual; if (s1 == L'❼' && s2 == L'❼' && s3 == L'❼') { winMultiplier = 100; winMessage = L"Джекпот! Три Семерки!"; }
    else if (s1 == L'⭐' && s2 == L'⭐' && s3 == L'⭐') { winMultiplier = 50; winMessage = L"Три Звезды!"; }
    else if (s1 == L'🔔' && s2 == L'🔔' && s3 == L'🔔') { winMultiplier = 20; winMessage = L"Три Колокольчика!"; }
    else if (s1 == L'🍉' && s2 == L'🍉' && s3 == L'🍉') { winMultiplier = 15; winMessage = L"Три Арбуза!"; }
    else if (s1 == L'🍊' && s2 == L'🍊' && s3 == L'🍊') { winMultiplier = 10; winMessage = L"Три Апельсина!"; }
    else if (s1 == L'🍋' && s2 == L'🍋' && s3 == L'🍋') { winMultiplier = 5; winMessage = L"Три Лимона!"; }
    else if (s1 == L'🍒' && s2 == L'🍒' && s3 == L'🍒') { winMultiplier = 3; winMessage = L"Три Вишни!"; }
    else { int cherryCount = 0; if (s1 == L'🍒') cherryCount++; if (s2 == L'🍒') cherryCount++; if (s3 == L'🍒') cherryCount++; if (cherryCount == 2) { winMultiplier = 1; winMessage = L"Две Вишни!"; } }
    double currentWin = betAmount * winMultiplier;

    InvalidateControlOnParent(hSlotsWinInfoLabel, hwnd); // ИНВАЛИДАЦИЯ НА РОДИТЕЛЕ

    if (currentWin > 0) {
        g_balance += currentWin;
        wchar_t winAmountStr[100];
        swprintf(winAmountStr, 100, L"Выигрыш: %.2f! %s", currentWin, winMessage.c_str());
        SetWindowText(hSlotsWinInfoLabel, winAmountStr);
        PlaySound(SOUND_WIN_FILE, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
    else {
        SetWindowText(hSlotsWinInfoLabel, winMessage.c_str());
    }
    UpdateBalanceLabelText(hwnd, hSlotsBalanceLabel); // Обновляем баланс после выигрыша
}

// Остальные функции CreateStage... DestroyStage... SaveUserDataToFile, IsNumeric (без изменений)
// Я их здесь опущу для краткости, но они должны быть в твоем коде.
// CreateStageInitialDepositControls, DestroyStageInitialDepositControls
// CreateStageGameSelectionControls, DestroyStageGameSelectionControls
// CreateStageRefillEnterAmountControls, DestroyStageRefillEnterAmountControls
// CreateStageSlotsGameControls, DestroyStageSlotsGameControls
// SaveUserDataToFile, IsNumeric
void CreateStageInitialDepositControls(HWND hwnd, bool fillUsernameAndLock) { HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT); HINSTANCE hInstance = GetModuleHandle(NULL); int yPos = 30; hUsernameLabel = CreateWindow(L"STATIC", L"Ваше имя:", WS_CHILD | WS_VISIBLE, 50, yPos, 150, 20, hwnd, (HMENU)IDC_USERNAME_LABEL, hInstance, NULL); hUsernameEdit = CreateWindow(L"EDIT", fillUsernameAndLock ? g_userName.c_str() : L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | (fillUsernameAndLock ? ES_READONLY : 0), 210, yPos, 250, 25, hwnd, (HMENU)IDC_USERNAME_EDIT, hInstance, NULL); yPos += 40; hCardLabel = CreateWindow(L"STATIC", L"Номер карты:", WS_CHILD | WS_VISIBLE, 50, yPos, 150, 20, hwnd, (HMENU)IDC_CARD_LABEL, hInstance, NULL); hCardEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 210, yPos, 250, 25, hwnd, (HMENU)IDC_CARD_EDIT, hInstance, NULL); yPos += 40; hCvvLabel = CreateWindow(L"STATIC", L"CVV:", WS_CHILD | WS_VISIBLE, 50, yPos, 150, 20, hwnd, (HMENU)IDC_CVV_LABEL, hInstance, NULL); hCvvEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 210, yPos, 70, 25, hwnd, (HMENU)IDC_CVV_EDIT, hInstance, NULL); yPos += 40; hAmountLabel = CreateWindow(L"STATIC", L"Сумма пополнения:", WS_CHILD | WS_VISIBLE, 50, yPos, 150, 20, hwnd, (HMENU)IDC_AMOUNT_LABEL, hInstance, NULL); hAmountEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 210, yPos, 150, 25, hwnd, (HMENU)IDC_AMOUNT_EDIT, hInstance, NULL); yPos += 50; hDepositButton = CreateWindow(L"BUTTON", L"Пополнить и войти", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 200, yPos, 200, 30, hwnd, (HMENU)IDC_DEPOSIT_BUTTON, hInstance, NULL); SendMessage(hUsernameLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hUsernameEdit, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hCardLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hCardEdit, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hCvvLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hCvvEdit, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hAmountLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hAmountEdit, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hDepositButton, WM_SETFONT, (WPARAM)hFont, TRUE); if (!fillUsernameAndLock) SetFocus(hUsernameEdit); else SetFocus(hCardEdit); }
void DestroyStageInitialDepositControls() { if (hUsernameLabel) DestroyWindow(hUsernameLabel); hUsernameLabel = NULL; if (hUsernameEdit) DestroyWindow(hUsernameEdit); hUsernameEdit = NULL; if (hCardLabel) DestroyWindow(hCardLabel); hCardLabel = NULL; if (hCardEdit) DestroyWindow(hCardEdit); hCardEdit = NULL; if (hCvvLabel) DestroyWindow(hCvvLabel); hCvvLabel = NULL; if (hCvvEdit) DestroyWindow(hCvvEdit); hCvvEdit = NULL; if (hAmountLabel) DestroyWindow(hAmountLabel); hAmountLabel = NULL; if (hAmountEdit) DestroyWindow(hAmountEdit); hAmountEdit = NULL; if (hDepositButton) DestroyWindow(hDepositButton); hDepositButton = NULL; }
void CreateStageGameSelectionControls(HWND hwnd) { HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT); HINSTANCE hInstance = GetModuleHandle(NULL); int yPos = 30; std::wstring welcomeText = L"Добро пожаловать, " + g_userName + L"!"; hWelcomeLabel = CreateWindow(L"STATIC", welcomeText.c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER, 50, yPos, 500, 25, hwnd, (HMENU)IDC_WELCOME_LABEL, hInstance, NULL); SendMessage(hWelcomeLabel, WM_SETFONT, (WPARAM)hFont, TRUE); yPos += 40; hBalanceLabel = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 50, yPos, 500, 25, hwnd, (HMENU)IDC_BALANCE_LABEL, hInstance, NULL); SendMessage(hBalanceLabel, WM_SETFONT, (WPARAM)hFont, TRUE); UpdateBalanceLabelText(hwnd, hBalanceLabel); yPos += 50; int buttonWidth = 180, buttonHeight = 40, buttonSpacing = 20; int startXSlotsRoulette = (600 - (2 * buttonWidth + buttonSpacing)) / 2; hSlotsButton = CreateWindow(L"BUTTON", L"🎰 Слоты 🎰", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, startXSlotsRoulette, yPos, buttonWidth, buttonHeight, hwnd, (HMENU)IDC_SLOTS_BUTTON, hInstance, NULL); hRouletteButton = CreateWindow(L"BUTTON", L"🎡 Рулетка 🎡", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, startXSlotsRoulette + buttonWidth + buttonSpacing, yPos, buttonWidth, buttonHeight, hwnd, (HMENU)IDC_ROULETTE_BUTTON, hInstance, NULL); yPos += buttonHeight + 30; int startXReplenishCashout = (600 - (2 * buttonWidth + buttonSpacing)) / 2; hReplenishAgainButton = CreateWindow(L"BUTTON", L"💰 Пополнить баланс", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, startXReplenishCashout, yPos, buttonWidth, buttonHeight, hwnd, (HMENU)IDC_REPLENISH_AGAIN_BUTTON, hInstance, NULL); hCashoutButton = CreateWindow(L"BUTTON", L"Вывести средства 🥔", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, startXReplenishCashout + buttonWidth + buttonSpacing, yPos, buttonWidth, buttonHeight, hwnd, (HMENU)IDC_CASHOUT_BUTTON, hInstance, NULL); SendMessage(hSlotsButton, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRouletteButton, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hReplenishAgainButton, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hCashoutButton, WM_SETFONT, (WPARAM)hFont, TRUE); }
void DestroyStageGameSelectionControls() { if (hWelcomeLabel) DestroyWindow(hWelcomeLabel); hWelcomeLabel = NULL; if (hBalanceLabel) DestroyWindow(hBalanceLabel); hBalanceLabel = NULL; if (hSlotsButton) DestroyWindow(hSlotsButton); hSlotsButton = NULL; if (hRouletteButton) DestroyWindow(hRouletteButton); hRouletteButton = NULL; if (hCashoutButton) DestroyWindow(hCashoutButton); hCashoutButton = NULL; if (hReplenishAgainButton) DestroyWindow(hReplenishAgainButton); hReplenishAgainButton = NULL; }
void CreateStageRefillEnterAmountControls(HWND hwnd) { HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT); HINSTANCE hInstance = GetModuleHandle(NULL); int yPos = 50; std::wstring infoText = L"Реквизиты: Карта ****" + (g_cardNumber.length() > 4 ? g_cardNumber.substr(g_cardNumber.length() - 4) : L"XXXX") + L", CVV сохранен."; hRefillInfoLabel = CreateWindow(L"STATIC", infoText.c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER, 50, yPos, 500, 25, hwnd, (HMENU)IDC_REFILL_INFO_LABEL, hInstance, NULL); yPos += 40; hRefillAmountLabel = CreateWindow(L"STATIC", L"Сумма пополнения:", WS_CHILD | WS_VISIBLE | SS_CENTER, 100, yPos, 150, 25, hwnd, (HMENU)IDC_REFILL_AMOUNT_LABEL, hInstance, NULL); hRefillAmountEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 260, yPos, 150, 25, hwnd, (HMENU)IDC_REFILL_AMOUNT_EDIT, hInstance, NULL); yPos += 50; hRefillConfirmButton = CreateWindow(L"BUTTON", L"Пополнить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 100, yPos, 120, 30, hwnd, (HMENU)IDC_REFILL_CONFIRM_BUTTON, hInstance, NULL); hRefillUseNewDetailsButton = CreateWindow(L"BUTTON", L"Новые реквизиты", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 240, yPos, 150, 30, hwnd, (HMENU)IDC_REFILL_USE_NEW_DETAILS_BUTTON, hInstance, NULL); hRefillCancelButton = CreateWindow(L"BUTTON", L"Отмена", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 410, yPos, 100, 30, hwnd, (HMENU)IDC_REFILL_CANCEL_BUTTON, hInstance, NULL); SendMessage(hRefillInfoLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRefillAmountLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRefillAmountEdit, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRefillConfirmButton, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRefillUseNewDetailsButton, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hRefillCancelButton, WM_SETFONT, (WPARAM)hFont, TRUE); SetFocus(hRefillAmountEdit); }
void DestroyStageRefillEnterAmountControls() { if (hRefillInfoLabel) DestroyWindow(hRefillInfoLabel); hRefillInfoLabel = NULL; if (hRefillAmountLabel) DestroyWindow(hRefillAmountLabel); hRefillAmountLabel = NULL; if (hRefillAmountEdit) DestroyWindow(hRefillAmountEdit); hRefillAmountEdit = NULL; if (hRefillConfirmButton) DestroyWindow(hRefillConfirmButton); hRefillConfirmButton = NULL; if (hRefillUseNewDetailsButton) DestroyWindow(hRefillUseNewDetailsButton); hRefillUseNewDetailsButton = NULL; if (hRefillCancelButton) DestroyWindow(hRefillCancelButton); hRefillCancelButton = NULL; }
void CreateStageSlotsGameControls(HWND hwnd) { HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT); HINSTANCE hInstance = GetModuleHandle(NULL); HFONT hReelFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial"); int yPos = 30; hSlotsBalanceLabel = CreateWindow(L"STATIC", L"Баланс: 0.00", WS_CHILD | WS_VISIBLE | SS_CENTER, 150, yPos, 300, 25, hwnd, (HMENU)IDC_SLOTS_BALANCE_LABEL, hInstance, NULL); SendMessage(hSlotsBalanceLabel, WM_SETFONT, (WPARAM)hFont, TRUE); UpdateBalanceLabelText(hwnd, hSlotsBalanceLabel); yPos += 40; int reelWidth = 60, reelSpacing = 20; int reelsTotalWidth = 3 * reelWidth + 2 * reelSpacing; int startXReels = (600 - reelsTotalWidth) / 2; hSlotsReel1 = CreateWindow(L"STATIC", L"⭐", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER, startXReels, yPos, reelWidth, 60, hwnd, (HMENU)IDC_SLOTS_REEL1_LABEL, hInstance, NULL); hSlotsReel2 = CreateWindow(L"STATIC", L"⭐", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER, startXReels + reelWidth + reelSpacing, yPos, reelWidth, 60, hwnd, (HMENU)IDC_SLOTS_REEL2_LABEL, hInstance, NULL); hSlotsReel3 = CreateWindow(L"STATIC", L"⭐", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER, startXReels + 2 * (reelWidth + reelSpacing), yPos, reelWidth, 60, hwnd, (HMENU)IDC_SLOTS_REEL3_LABEL, hInstance, NULL); SendMessage(hSlotsReel1, WM_SETFONT, (WPARAM)hReelFont, TRUE); SendMessage(hSlotsReel2, WM_SETFONT, (WPARAM)hReelFont, TRUE); SendMessage(hSlotsReel3, WM_SETFONT, (WPARAM)hReelFont, TRUE); yPos += 60 + 30; hSlotsBetPromptLabel = CreateWindow(L"STATIC", L"Ваша ставка:", WS_CHILD | WS_VISIBLE | SS_RIGHT, 150, yPos, 100, 25, hwnd, (HMENU)IDC_SLOTS_BET_PROMPT_LABEL, hInstance, NULL); hSlotsBetEdit = CreateWindow(L"EDIT", L"10", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL, 260, yPos, 100, 25, hwnd, (HMENU)IDC_SLOTS_BET_EDIT, hInstance, NULL); SendMessage(hSlotsBetPromptLabel, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hSlotsBetEdit, WM_SETFONT, (WPARAM)hFont, TRUE); yPos += 25 + 20; hSlotsSpinButton = CreateWindow(L"BUTTON", L"КРУТИТЬ!", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 200, yPos, 200, 40, hwnd, (HMENU)IDC_SLOTS_SPIN_BUTTON, hInstance, NULL); SendMessage(hSlotsSpinButton, WM_SETFONT, (WPARAM)hFont, TRUE); yPos += 40 + 20; hSlotsWinInfoLabel = CreateWindow(L"STATIC", L"Удачи!", WS_CHILD | WS_VISIBLE | SS_CENTER, 100, yPos, 400, 25, hwnd, (HMENU)IDC_SLOTS_WIN_INFO_LABEL, hInstance, NULL); SendMessage(hSlotsWinInfoLabel, WM_SETFONT, (WPARAM)hFont, TRUE); yPos += 25 + 30; hSlotsBackButton = CreateWindow(L"BUTTON", L"Назад в меню", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 225, yPos, 150, 30, hwnd, (HMENU)IDC_SLOTS_BACK_BUTTON, hInstance, NULL); SendMessage(hSlotsBackButton, WM_SETFONT, (WPARAM)hFont, TRUE); if (hReelFont) DeleteObject(hReelFont); }
void DestroyStageSlotsGameControls() { if (hSlotsReel1) DestroyWindow(hSlotsReel1); hSlotsReel1 = NULL; if (hSlotsReel2) DestroyWindow(hSlotsReel2); hSlotsReel2 = NULL; if (hSlotsReel3) DestroyWindow(hSlotsReel3); hSlotsReel3 = NULL; if (hSlotsBetPromptLabel) DestroyWindow(hSlotsBetPromptLabel); hSlotsBetPromptLabel = NULL; if (hSlotsBetEdit) DestroyWindow(hSlotsBetEdit); hSlotsBetEdit = NULL; if (hSlotsSpinButton) DestroyWindow(hSlotsSpinButton); hSlotsSpinButton = NULL; if (hSlotsWinInfoLabel) DestroyWindow(hSlotsWinInfoLabel); hSlotsWinInfoLabel = NULL; if (hSlotsBackButton) DestroyWindow(hSlotsBackButton); hSlotsBackButton = NULL; if (hSlotsBalanceLabel) DestroyWindow(hSlotsBalanceLabel); hSlotsBalanceLabel = NULL; }
void SaveUserDataToFile(double lastDepositAmount) { if (g_userName.empty()) return; std::wstring filename = g_userName + L".txt"; std::wofstream outFile(filename); if (outFile.is_open()) { outFile.imbue(std::locale("")); outFile << L"Имя пользователя: " << g_userName << std::endl; outFile << L"Номер карты: " << g_cardNumber << std::endl; outFile << L"CVV: " << g_cardCVV << std::endl; wchar_t balanceStr[50]; swprintf(balanceStr, 50, L"%.2f", g_balance); outFile << L"Текущий общий баланс: " << balanceStr << std::endl; wchar_t depositStr[50]; swprintf(depositStr, 50, L"%.2f", lastDepositAmount); outFile << L"Сумма последнего пополнения: " << depositStr << std::endl; SYSTEMTIME st; GetLocalTime(&st); wchar_t timeStr[100]; swprintf(timeStr, 100, L"%02d.%02d.%04d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond); outFile << L"Данные записаны: " << timeStr << std::endl; outFile.close(); } else { MessageBox(NULL, (L"Не удалось сохранить данные в файл: " + filename).c_str(), L"Ошибка файла", MB_OK | MB_ICONERROR); } }
bool IsNumeric(const std::wstring& s) { if (s.empty()) return false; return std::all_of(s.begin(), s.end(), ::iswdigit); }