#pragma once

#include <Arduino.h>

// Supported languages
enum Language_t {
    LANG_EN = 0,
    LANG_DE = 1,
    LANG_CS = 2,
    LANG_PL = 3,
    LANG_COUNT = 4
};

// String IDs for localization
enum StringID {
    // Day names - short (2 letters)
    STR_DAY_SUN_SHORT,
    STR_DAY_MON_SHORT,
    STR_DAY_TUE_SHORT,
    STR_DAY_WED_SHORT,
    STR_DAY_THU_SHORT,
    STR_DAY_FRI_SHORT,
    STR_DAY_SAT_SHORT,
    
    // Day names - full
    STR_DAY_SUN_FULL,
    STR_DAY_MON_FULL,
    STR_DAY_TUE_FULL,
    STR_DAY_WED_FULL,
    STR_DAY_THU_FULL,
    STR_DAY_FRI_FULL,
    STR_DAY_SAT_FULL,
    
    // Month names - full
    STR_MONTH_JAN,
    STR_MONTH_FEB,
    STR_MONTH_MAR,
    STR_MONTH_APR,
    STR_MONTH_MAY,
    STR_MONTH_JUN,
    STR_MONTH_JUL,
    STR_MONTH_AUG,
    STR_MONTH_SEP,
    STR_MONTH_OCT,
    STR_MONTH_NOV,
    STR_MONTH_DEC,
    
    // Month names - short
    STR_MONTH_JAN_SHORT,
    STR_MONTH_FEB_SHORT,
    STR_MONTH_MAR_SHORT,
    STR_MONTH_APR_SHORT,
    STR_MONTH_MAY_SHORT,
    STR_MONTH_JUN_SHORT,
    STR_MONTH_JUL_SHORT,
    STR_MONTH_AUG_SHORT,
    STR_MONTH_SEP_SHORT,
    STR_MONTH_OCT_SHORT,
    STR_MONTH_NOV_SHORT,
    STR_MONTH_DEC_SHORT,
    
    // Weather descriptions
    STR_WEATHER_CLEAR,
    STR_WEATHER_MAINLY_CLEAR,
    STR_WEATHER_PARTLY_CLOUDY,
    STR_WEATHER_OVERCAST,
    STR_WEATHER_FOG,
    STR_WEATHER_DRIZZLE,
    STR_WEATHER_RAIN,
    STR_WEATHER_HEAVY_RAIN,
    STR_WEATHER_FREEZING_RAIN,
    STR_WEATHER_SNOW,
    STR_WEATHER_HEAVY_SNOW,
    STR_WEATHER_SHOWERS,
    STR_WEATHER_THUNDERSTORM,
    STR_WEATHER_UNKNOWN,
    
    // Labels
    STR_LABEL_HUMIDITY,
    STR_LABEL_WIND,
    STR_LABEL_PRECIPITATION,
    STR_LABEL_PRESSURE,
    STR_LABEL_FORECAST,
    STR_LABEL_TODAY,
    STR_LABEL_TOMORROW,
    STR_LABEL_WEATHER_DASHBOARD,
    STR_LABEL_TODAYS_FORECAST,
    STR_LABEL_3DAY_OUTLOOK,
    STR_LABEL_UV_INDEX,
    STR_LABEL_SUNRISE_SUNSET,
    STR_LABEL_SUNRISE,
    STR_LABEL_SUNSET,
    STR_LABEL_HIGH,
    STR_LABEL_LOW,
    
    // Time periods
    STR_TIME_MORNING,
    STR_TIME_AFTERNOON,
    STR_TIME_EVENING,
    STR_TIME_NIGHT,
    
    // UV Levels
    STR_UV_LOW,
    STR_UV_MODERATE,
    STR_UV_HIGH,
    STR_UV_VERY_HIGH,
    STR_UV_EXTREME,
    
    // Status messages
    STR_STATUS_STARTING,
    STR_STATUS_FACTORY_RESET_HINT,
    STR_STATUS_FACTORY_RESET_PROGRESS,
    STR_STATUS_CONNECT_WIFI,
    STR_STATUS_OPEN_BROWSER,
    STR_STATUS_CONNECTING,
    STR_STATUS_UPDATING_FIRMWARE,
    STR_STATUS_LOADING_WEATHER,
    STR_STATUS_CONNECTED_TO,
    STR_STATUS_LOADING_DATA,
    STR_STATUS_PRESS_RESET,
    
    // Weather UI strings
    STR_WEATHER_FEELS_LIKE,
    STR_WEATHER_HUMIDITY,
    STR_WEATHER_WIND,
    STR_WEATHER_HOURLY,
    STR_WEATHER_DAILY,
    STR_WEATHER_UPDATED,
    
    // Low battery
    STR_STATUS_LOW_BATTERY,
    STR_STATUS_CONNECT_CHARGER,
    
    STR_COUNT
};

class Localization {
public:
    static void setLanguage(Language_t lang);
    static Language_t getLanguage();
    static const char* get(StringID id);
    static const char* getLanguageCode();
    
    static const char* getDayShort(int dayOfWeek);
    static const char* getDayFull(int dayOfWeek);
    static const char* getMonth(int month);
    static const char* getMonthShort(int month);
    
private:
    static Language_t& currentLanguageRef();
};

// English strings
static const char* const stringsEN[] = {
    // Day names - short
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
    // Day names - full
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
    // Month names - full
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
    // Month names - short
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    // Weather
    "Clear", "Mainly clear", "Partly cloudy", "Overcast", "Fog",
    "Drizzle", "Rain", "Heavy rain", "Freezing rain",
    "Snow", "Heavy snow", "Showers", "Thunderstorm", "Unknown",
    // Labels
    "Humidity", "Wind", "Precipitation", "Pressure", "Forecast", "Today", "Tomorrow",
    "Weather Dashboard", "Today's Forecast", "3-Day Outlook",
    "UV Index", "Sunrise & Sunset", "Sunrise", "Sunset", "High", "Low",
    // Time periods
    "Morning", "Afternoon", "Evening", "Night",
    // UV Levels
    "Low", "Moderate", "High", "Very High", "Extreme",
    // Status
    "Starting...", "(Press RESET for factory reset)", "Factory reset in progress...",
    "Connect to WiFi:", "Open in browser:", "Connecting...",
    "Updating firmware...", "Loading weather...",
    "Connected to", "Loading data...", "Press RESET to retry",
    // Weather UI
    "Feels like", "Humidity", "Wind", "Hourly", "Daily", "Updated",
    // Low battery
    "Low battery...", "Connect to charger."
};

// German strings
static const char* const stringsDE[] = {
    // Day names - short
    "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa",
    // Day names - full
    "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag",
    // Month names - full
    "Januar", "Februar", "März", "April", "Mai", "Juni",
    "Juli", "August", "September", "Oktober", "November", "Dezember",
    // Month names - short
    "Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez",
    // Weather
    "Klar", "Überwiegend klar", "Teilweise bewölkt", "Bewölkt", "Nebel",
    "Nieselregen", "Regen", "Starkregen", "Gefrierender Regen",
    "Schnee", "Starker Schnee", "Schauer", "Gewitter", "Unbekannt",
    // Labels
    "Feuchtigkeit", "Wind", "Niederschlag", "Luftdruck", "Vorhersage", "Heute", "Morgen",
    "Wetter Dashboard", "Heutige Vorhersage", "3-Tage Ausblick",
    "UV-Index", "Sonnenauf- & untergang", "Sonnenaufgang", "Sonnenuntergang", "Höchst", "Tiefst",
    // Time periods
    "Morgen", "Nachmittag", "Abend", "Nacht",
    // UV Levels
    "Niedrig", "Mäßig", "Hoch", "Sehr hoch", "Extrem",
    // Status
    "Starten...", "(RESET drücken für Werkseinstellung)", "Werkseinstellung läuft...",
    "Mit WLAN verbinden:", "Im Browser öffnen:", "Verbinde...",
    "Firmware-Update...", "Wetter laden...",
    "Verbunden mit", "Daten werden geladen...", "RESET drücken zum Wiederholen",
    // Weather UI
    "Gefühlt", "Feuchtigkeit", "Wind", "Stündlich", "Täglich", "Aktualisiert",
    // Low battery
    "Batterie schwach...", "An Ladegerät anschließen."
};

// Czech strings
static const char* const stringsCS[] = {
    // Day names - short
    "Ne", "Po", "Út", "St", "Čt", "Pá", "So",
    // Day names - full
    "Neděle", "Pondělí", "Úterý", "Středa", "Čtvrtek", "Pátek", "Sobota",
    // Month names - full
    "Leden", "Únor", "Březen", "Duben", "Květen", "Červen",
    "Červenec", "Srpen", "Září", "Říjen", "Listopad", "Prosinec",
    // Month names - short
    "led", "úno", "bře", "dub", "kvě", "čvn", "čvc", "srp", "zář", "říj", "lis", "pro",
    // Weather
    "Jasno", "Převážně jasno", "Polojasno", "Zataženo", "Mlha",
    "Mrholení", "Déšť", "Silný déšť", "Mrznoucí déšť",
    "Sněžení", "Husté sněžení", "Přeháňky", "Bouřka", "Neznámé",
    // Labels
    "Vlhkost", "Vítr", "Srážky", "Tlak", "Předpověď", "Dnes", "Zítra",
    "Přehled počasí", "Dnešní předpověď", "Výhled na 3 dny",
    "UV index", "Východ & západ slunce", "Východ", "Západ", "Max", "Min",
    // Time periods
    "Ráno", "Odpoledne", "Večer", "Noc",
    // UV Levels
    "Nízký", "Střední", "Vysoký", "Velmi vysoký", "Extrémní",
    // Status
    "Startuji...", "(Stiskněte RESET pro tovární nastavení)", "Probíhá tovární nastavení...",
    "Připojte se k WiFi:", "Otevřete v prohlížeči:", "Připojuji...",
    "Aktualizuji firmware...", "Načítám počasí...",
    "Připojeno k", "Načítám data...", "Stiskněte RESET pro opakování",
    // Weather UI
    "Pocitově", "Vlhkost", "Vítr", "Hodinová", "Denní", "Aktualizováno",
    // Low battery
    "Slabá baterie...", "Připojte nabíječku."
};

// Polish strings
static const char* const stringsPL[] = {
    // Day names - short
    "Nd", "Pn", "Wt", "Śr", "Cz", "Pt", "So",
    // Day names - full
    "Niedziela", "Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota",
    // Month names - full
    "Styczeń", "Luty", "Marzec", "Kwiecień", "Maj", "Czerwiec",
    "Lipiec", "Sierpień", "Wrzesień", "Październik", "Listopad", "Grudzień",
    // Month names - short
    "sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paź", "lis", "gru",
    // Weather
    "Bezchmurnie", "Przeważnie jasno", "Częściowo pochmurno", "Pochmurno", "Mgła",
    "Mżawka", "Deszcz", "Silny deszcz", "Marznący deszcz",
    "Śnieg", "Intensywny śnieg", "Przelotne opady", "Burza", "Nieznane",
    // Labels
    "Wilgotność", "Wiatr", "Opady", "Ciśnienie", "Prognoza", "Dziś", "Jutro",
    "Przegląd pogody", "Dzisiejsza prognoza", "Prognoza 3-dniowa",
    "Indeks UV", "Wschód & zachód słońca", "Wschód", "Zachód", "Maks", "Min",
    // Time periods
    "Rano", "Popołudnie", "Wieczór", "Noc",
    // UV Levels
    "Niski", "Umiarkowany", "Wysoki", "Bardzo wysoki", "Ekstremalny",
    // Status
    "Uruchamianie...", "(Naciśnij RESET aby przywrócić ustawienia)", "Przywracanie ustawień...",
    "Połącz z WiFi:", "Otwórz w przeglądarce:", "Łączenie...",
    "Aktualizacja firmware...", "Ładowanie pogody...",
    "Połączono z", "Ładowanie danych...", "Naciśnij RESET aby ponowić",
    // Weather UI
    "Odczuwalna", "Wilgotność", "Wiatr", "Godzinowa", "Dzienna", "Zaktualizowano",
    // Low battery
    "Słaba bateria...", "Podłącz ładowarkę."
};

static const char* const* stringTables[] = {
    stringsEN,
    stringsDE,
    stringsCS,
    stringsPL
};

static const char* languageCodes[] = {
    "en", "de", "cs", "pl"
};

inline Language_t& Localization::currentLanguageRef() {
    static Language_t lang = LANG_CS;
    return lang;
}

inline void Localization::setLanguage(Language_t lang) {
    if (lang < LANG_COUNT) {
        currentLanguageRef() = lang;
    }
}

inline Language_t Localization::getLanguage() {
    return currentLanguageRef();
}

inline const char* Localization::get(StringID id) {
    if (id >= STR_COUNT) return "";
    return stringTables[currentLanguageRef()][id];
}

inline const char* Localization::getLanguageCode() {
    return languageCodes[currentLanguageRef()];
}

inline const char* Localization::getDayShort(int dayOfWeek) {
    if (dayOfWeek < 0 || dayOfWeek > 6) return "";
    return get((StringID)(STR_DAY_SUN_SHORT + dayOfWeek));
}

inline const char* Localization::getDayFull(int dayOfWeek) {
    if (dayOfWeek < 0 || dayOfWeek > 6) return "";
    return get((StringID)(STR_DAY_SUN_FULL + dayOfWeek));
}

inline const char* Localization::getMonth(int month) {
    if (month < 0 || month > 11) return "";
    return get((StringID)(STR_MONTH_JAN + month));
}

inline const char* Localization::getMonthShort(int month) {
    if (month < 0 || month > 11) return "";
    return get((StringID)(STR_MONTH_JAN_SHORT + month));
}
