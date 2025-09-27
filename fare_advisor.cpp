// fare_advisor.cpp
// E-Hailing Fare Advisor (C++)
// Compile: g++ -std=c++11 fare_advisor.cpp -o fare_advisor

#include <bits/stdc++.h>
using namespace std;

const string WIDGET_FILE = "widgets.txt";
const double BASE_CHARGE = 2.0;
const double PER_KM_RATE = 1.2;

struct Widget {
    string name;
    string start;
    string end;
    double distance;
    double base_fare; // -1 if auto
    int preferred_hour; // -1 if none
    string day_type; // "weekday", "weekend", or "" for auto
    bool is_holiday;
};

static double round2(double x) {
    return std::round(x * 100.0) / 100.0;
}

pair<double, string> get_multiplier(int hour, const string& day_type, bool is_holiday) {
    if (is_holiday) return {2.0, "Holiday surge"};
    if (day_type == "weekday") {
        if (7 <= hour && hour <= 9) return {1.8, "Morning rush"};
        if (17 <= hour && hour <= 20) return {1.6, "Evening peak"};
        if (0 <= hour && hour <= 5) return {0.9, "Late night off-peak"};
        return {1.0, "Normal"};
    } else { // weekend
        if (18 <= hour && hour <= 23) return {1.5, "Weekend evening surge"};
        if (0 <= hour && hour <= 5) return {0.95, "Late night (cheaper)"};
        return {1.0, "Normal"};
    }
}

map<string, string> estimate_cost(double distance, int hour, string day_type, double base_fare_val, bool is_holiday) {
    if (day_type != "weekday" && day_type != "weekend") {
        // decide by current system day
        time_t t = time(nullptr);
        tm* now = localtime(&t);
        day_type = (now->tm_wday < 6 && now->tm_wday >= 1) ? "weekday" : "weekend";
    }
    double base_cost = (base_fare_val >= 0.0) ? base_fare_val : (BASE_CHARGE + PER_KM_RATE * distance);
    auto mult_pair = get_multiplier(hour, day_type, is_holiday);
    double mult = mult_pair.first;
    string reason = mult_pair.second;
    double estimated = round2(base_cost * mult);

    string status, tip;
    if (mult >= 1.6) {
        status = "Expensive";
        tip = "High demand — consider waiting or different time.";
    } else if (mult > 1.0) {
        status = "Normal-High";
        tip = "Moderate demand.";
    } else if (mult == 1.0) {
        status = "Normal";
        tip = "Good time to book.";
    } else {
        status = "Cheap";
        tip = "Off-peak — good time to book.";
    }

    map<string, string> res;
    res["estimated"] = to_string(estimated);
    res["multiplier"] = to_string(mult);
    res["reason"] = reason;
    res["status"] = status;
    res["tip"] = tip;
    res["hour"] = to_string(hour);
    res["day_type"] = day_type;
    return res;
}

/* Widgets persistence: simple pipe-separated fields per line */
vector<Widget> load_widgets() {
    vector<Widget> list;
    ifstream fin(WIDGET_FILE);
    if (!fin.is_open()) return list;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> parts;
        while (getline(ss, token, '|')) parts.push_back(token);
        if (parts.size() != 8) continue;
        Widget w;
        w.name = parts[0];
        w.start = parts[1];
        w.end = parts[2];
        w.distance = stod(parts[3]);
        w.base_fare = stod(parts[4]);
        w.preferred_hour = stoi(parts[5]);
        w.day_type = parts[6];
        w.is_holiday = (parts[7] == "1");
        list.push_back(w);
    }
    fin.close();
    return list;
}

void save_widgets(const vector<Widget>& list) {
    ofstream fout(WIDGET_FILE, ios::trunc);
    for (const auto &w : list) {
        fout << w.name << "|" << w.start << "|" << w.end << "|"
             << w.distance << "|" << w.base_fare << "|"
             << w.preferred_hour << "|" << w.day_type << "|"
             << (w.is_holiday ? "1" : "0") << "\n";
    }
    fout.close();
}

void add_widget(vector<Widget>& list) {
    Widget w;
    cout << "Widget name (e.g., Home->Work): ";
    getline(cin, w.name);
    if (w.name.empty()) { cout << "اسم غير صالح.\n"; return; }
    cout << "Start location: "; getline(cin, w.start);
    cout << "End location: "; getline(cin, w.end);
    cout << "Distance (km): ";
    string tmp; getline(cin, tmp); w.distance = stod(tmp.empty() ? "0" : tmp);
    cout << "Base fare RM (enter -1 for auto): ";
    getline(cin, tmp); w.base_fare = stod(tmp.empty() ? "-1" : tmp);
    cout << "Preferred hour (0-23) or -1 for now: "; getline(cin, tmp); w.preferred_hour = stoi(tmp.empty() ? "-1" : tmp);
    cout << "Day type (weekday/weekend) or 'auto' for now: "; getline(cin, w.day_type);
    if (w.day_type != "weekday" && w.day_type != "weekend") w.day_type = "";
    cout << "Is this a holiday? (y/n): "; getline(cin, tmp); w.is_holiday = (tmp == "y" || tmp == "Y");
    list.push_back(w);
    save_widgets(list);
    cout << "✅ Widget '" << w.name << "' saved.\n";
}

void delete_widget(vector<Widget>& list) {
    cout << "Enter widget name to delete: ";
    string name; getline(cin, name);
    auto it = remove_if(list.begin(), list.end(), [&](const Widget& w){ return w.name == name; });
    if (it != list.end()) {
        list.erase(it, list.end());
        save_widgets(list);
        cout << "Deleted.\n";
    } else {
        cout << "No such widget.\n";
    }
}

void view_widgets_now(const vector<Widget>& list) {
    if (list.empty()) { cout << "No widgets saved.\n"; return; }
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    int cur_hour = now->tm_hour;
    string cur_day = (now->tm_wday < 6 && now->tm_wday >= 1) ? "weekday" : "weekend";
    cout << "\nYour Widgets (current estimation):\n";
    for (const auto& w : list) {
        int hour = (w.preferred_hour >= 0) ? w.preferred_hour : cur_hour;
        string day = (!w.day_type.empty()) ? w.day_type : cur_day;
        double baseFare = w.base_fare;
        map<string,string> res = estimate_cost(w.distance, hour, day, baseFare, w.is_holiday);
        cout << "- " << w.name << ": " << w.start << " -> " << w.end
             << " | " << res["status"] << " | RM " << res["estimated"]
             << " | " << res["reason"] << " | Tip: " << res["tip"] << "\n";
    }
}

void view_widget_detail(const vector<Widget>& list) {
    cout << "Widget name: ";
    string name; getline(cin, name);
    for (const auto& w : list) {
        if (w.name == name) {
            time_t t = time(nullptr);
            tm* now = localtime(&t);
            int cur_hour = now->tm_hour;
            string cur_day = (now->tm_wday < 6 && now->tm_wday >= 1) ? "weekday" : "weekend";
            int hour = (w.preferred_hour >= 0) ? w.preferred_hour : cur_hour;
            string day = (!w.day_type.empty()) ? w.day_type : cur_day;
            map<string,string> res = estimate_cost(w.distance, hour, day, w.base_fare, w.is_holiday);
            cout << "\n" << w.name << ": " << w.start << " -> " << w.end << "\n";
            cout << "Distance: " << w.distance << " km\n";
            cout << "Estimated now: RM " << res["estimated"] << " (" << res["status"] << ")\n";
            cout << "Reason: " << res["reason"] << "\nTip: " << res["tip"] << "\n\n";
            return;
        }
    }
    cout << "No such widget.\n";
}

void estimate_ad_hoc() {
    cout << "Distance (km): ";
    string tmp; getline(cin, tmp); double distance = stod(tmp.empty() ? "0" : tmp);
    cout << "Base fare RM (enter -1 for auto): "; getline(cin, tmp); double baseFare = stod(tmp.empty() ? "-1" : tmp);
    cout << "Hour (0-23) or -1 for now: "; getline(cin, tmp); int hour = stoi(tmp.empty() ? "-1" : tmp);
    cout << "Day type (weekday/weekend) or auto: ";
    string day_type; getline(cin, day_type);
    if (day_type != "weekday" && day_type != "weekend") day_type = "";
    cout << "Holiday? (y/n): "; getline(cin, tmp); bool is_holiday = (tmp=="y"||tmp=="Y");
    time_t t = time(nullptr); tm* now = localtime(&t);
    if (hour < 0) hour = now->tm_hour;
    if (day_type.empty()) day_type = (now->tm_wday < 6 && now->tm_wday >= 1) ? "weekday" : "weekend";
    auto res = estimate_cost(distance, hour, day_type, baseFare, is_holiday);
    cout << "Estimated: RM " << res["estimated"] << " | " << res["status"] << " | Reason: " << res["reason"] << " | Tip: " << res["tip"] << "\n";
}

int main() {
    vector<Widget> widgets = load_widgets();
    while (true) {
        cout << "\n=== E-Hailing Fare Advisor (C++) ===\n";
        cout << "1) Estimate ad-hoc\n2) Add widget\n3) View widgets (current)\n4) View widget detail\n5) Delete widget\n6) Exit\nChoice: ";
        string choice;
        getline(cin, choice);
        if (choice == "1") estimate_ad_hoc();
        else if (choice == "2") add_widget(widgets);
        else if (choice == "3") view_widgets_now(widgets);
        else if (choice == "4") view_widget_detail(widgets);
        else if (choice == "5") delete_widget(widgets);
        else if (choice == "6") { cout << "Goodbye.\n"; break; }
        else cout << "Invalid choice.\n";
    }
    return 0;
}
