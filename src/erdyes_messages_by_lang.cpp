#include "erdyes_messages.hpp"

std::map<std::string, erdyes::messages_type> erdyes::messages_by_lang = {
    {"english",
     {
         .apply_dyes = L"Apply dyes",
         .primary_color = L"Primary color",
         .secondary_color = L"Secondary color",
         .tertiary_color = L"Tertiary color",
         .primary_intensity = L"Primary intensity",
         .secondary_intensity = L"Secondary intensity",
         .tertiary_intensity = L"Tertiary intensity",
         .none = L"None",
         .back = L"Back",
     }},
    {"german",
     {
         .apply_dyes = L"Farbe auftragen",
         .primary_color = L"Farbe 1",
         .secondary_color = L"Farbe 2",
         .tertiary_color = L"Farbe 3",
         .primary_intensity = L"Stärke 1",
         .secondary_intensity = L"Stärke 2",
         .tertiary_intensity = L"Stärke 3",
         .none = L"Keine",
         .back = L"Zurück",
     }},
    {"french",
     {
         .apply_dyes = L"Appliquer les couleurs",
         .primary_color = L"Couleur 1",
         .secondary_color = L"Couleur 2",
         .tertiary_color = L"Couleur 3",
         .primary_intensity = L"Intensité 1",
         .secondary_intensity = L"Intensité 2",
         .tertiary_intensity = L"Intensité 3",
         .none = L"Aucune",
         .back = L"Retour",
     }},
    {"italian",
     {
         .apply_dyes = L"Applica colori",
         .primary_color = L"Colore 1",
         .secondary_color = L"Colore 2",
         .tertiary_color = L"Colore 3",
         .primary_intensity = L"Intensità 1",
         .secondary_intensity = L"Intensità 2",
         .tertiary_intensity = L"Intensità 3",
         .none = L"Nessuno",
         .back = L"Indietro",
     }},
    {"japanese",
     {
         .apply_dyes = L"Apply dyes", // TODO translate
         .primary_color = L"カラー 1",
         .secondary_color = L"カラー 2",
         .tertiary_color = L"カラー 3",
         .primary_intensity = L"濃さ 1",
         .secondary_intensity = L"濃さ 2",
         .tertiary_intensity = L"濃さ 3",
         .none = L"なし",
         .back = L"戻る",
     }},
    {"koreana",
     {
         .apply_dyes = L"Apply dyes", // TODO translate
         .primary_color = L"색상 1",
         .secondary_color = L"색상 2",
         .tertiary_color = L"색상 3",
         .primary_intensity = L"짙음 1",
         .secondary_intensity = L"짙음 2",
         .tertiary_intensity = L"짙음 3",
         .none = L"없음",
         .back = L"돌아가기",
     }},
    {"polish",
     {
         .apply_dyes = L"Zastosuj kolory",
         .primary_color = L"Kolor 1",
         .secondary_color = L"Kolor 2",
         .tertiary_color = L"Kolor 3",
         .primary_intensity = L"Wyrazistość 1",
         .secondary_intensity = L"Wyrazistość 2",
         .tertiary_intensity = L"Wyrazistość 3",
         .none = L"Brak",
         .back = L"Wróć",
     }},
    {"brazilian",
     {
         .apply_dyes = L"Aplicar cores",
         .primary_color = L"Cor 1",
         .secondary_color = L"Cor 2",
         .tertiary_color = L"Cor 3",
         .primary_intensity = L"Intensidade 1",
         .secondary_intensity = L"Intensidade 2",
         .tertiary_intensity = L"Intensidade 3",
         .none = L"Nenhuma",
         .back = L"Voltar",
     }},
    {"russian",
     {
         .apply_dyes = L"Apply dyes", // TODO translate
         .primary_color = L"Цвет 1",
         .secondary_color = L"Цвет 2",
         .tertiary_color = L"Цвет 3",
         .primary_intensity = L"яркость 1",
         .secondary_intensity = L"яркость 2",
         .tertiary_intensity = L"яркость 3",
         .none = L"Нет",
         .back = L"Назад",
     }},
    {"latam",
     {
         .apply_dyes = L"Aplicar colores",
         .primary_color = L"Color 1",
         .secondary_color = L"Color 2",
         .tertiary_color = L"Color 3",
         .primary_intensity = L"Intensidad 1",
         .secondary_intensity = L"Intensidad 2",
         .tertiary_intensity = L"Intensidad 3",
         .none = L"Ninguno",
         .back = L"Atrás",
     }},
    {"spanish",
     {
         .apply_dyes = L"Aplicar colores",
         .primary_color = L"Color 1",
         .secondary_color = L"Color 2",
         .tertiary_color = L"Color 3",
         .primary_intensity = L"Intensidad 1",
         .secondary_intensity = L"Intensidad 2",
         .tertiary_intensity = L"Intensidad 3",
         .none = L"Ninguno",
         .back = L"Volver",
     }},
    {"schinese",
     {
         .apply_dyes = L"Apply dyes", // TODO translate
         .primary_color = L"色彩 1",
         .secondary_color = L"色彩 2",
         .tertiary_color = L"色彩 3",
         .primary_intensity = L"浓淡 1",
         .secondary_intensity = L"浓淡 2",
         .tertiary_intensity = L"浓淡 3",
         .none = L"无",
         .back = L"返回",
     }},
    {"tchinese",
     {
         .apply_dyes = L"Apply dyes", // TODO translate
         .primary_color = L"色彩 1",
         .secondary_color = L"色彩 2",
         .tertiary_color = L"色彩 3",
         .primary_intensity = L"濃淡 1",
         .secondary_intensity = L"濃淡 2",
         .tertiary_intensity = L"濃淡 3",
         .none = L"無",
         .back = L"返回",
     }},
    {"thai",
     {
         .apply_dyes = L"Apply dyes",                   // TODO translate
         .primary_color = L"Primary color",             // TODO translate
         .secondary_color = L"Secondary color",         // TODO translate
         .tertiary_color = L"Tertiary color",           // TODO translate
         .primary_intensity = L"Primary intensity",     // TODO translate
         .secondary_intensity = L"Secondary intensity", // TODO translate
         .tertiary_intensity = L"Tertiary intensity",   // TODO translate
         .none = L"ไม่มี",
         .back = L"ย้อน​กลับ",
     }},
    {"arabic",
     {
         .apply_dyes = L"Apply dyes",                   // TODO translate
         .primary_color = L"Primary color",             // TODO translate
         .secondary_color = L"Secondary color",         // TODO translate
         .tertiary_color = L"Tertiary color",           // TODO translate
         .primary_intensity = L"Primary intensity",     // TODO translate
         .secondary_intensity = L"Secondary intensity", // TODO translate
         .tertiary_intensity = L"Tertiary intensity",   // TODO translate
         .none = L"ﻻ ﺷﻲء",
         .back = L"ﻋﻭدة",
     }},
};
