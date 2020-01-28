#include <FastLED.h>
#include <TM1637Display.h>

// Total number of pins:
// Input:
//     3 Generate buttons
//     2 Search buttons
//     6 Sort buttons
//     2 pins for encoder
// Output:
//     2 for LED strips
//     2 for 7 seg display

#define STEP_CLK 18
#define STEP_DIO 19

#define SPEED_A_PIN 22
#define SPEED_B_PIN 21

#define VALUES_LED_PIN     2
#define INDICATORS_LED_PIN 4

// -- Algorithms
enum Choice {
    None, GenRandom, GenNoise, Reverse,
    LinearSearch, BinarySearch, Reserved,
    BubbleSort, InsertionSort, QuickSort, MergeSort, HeapSort, BitonicSort,
    StartPause
};

struct Button
{
    int pin;
    Choice choice;
    int sense;
    bool clicked;
    int last_click_time;
};

#define NUM_BUTTONS 13
Button gButtons[NUM_BUTTONS] = {
    { 26, GenRandom, false },
    { 25, GenNoise, false },
    { 35, Reverse, false },
    { 12, LinearSearch, false },
    { 14, BinarySearch, false },
    { 27, Reserved, false },
    { 38, BubbleSort, false },
    { 37, InsertionSort, false },
    { 36, QuickSort, false },
    { 33, MergeSort, false },
    { 32, HeapSort, false },
    { 39, BitonicSort, false },
    { 34, StartPause, false },
};

bool is_algorithm(Choice c)
{
    return (c >= LinearSearch and c <= BitonicSort);
}

Choice check_buttons()
{
    static uint32_t last_check = 0;

    uint32_t t = millis();
    if (t - last_check > 40) {
        last_check = t;
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (digitalRead(gButtons[i].pin) == HIGH) {
                gButtons[i].sense++;
                if (gButtons[i].sense == 2) {
                    gButtons[i].clicked = true;
                    gButtons[i].last_click_time = millis();
                    Serial.println(gButtons[i].pin);
                    return gButtons[i].choice;
                }
            } else {
                gButtons[i].sense = 0;
            }
        }
    }
    
    return None;
}

// -- Program state
enum State { RUN, PAUSE, STOP };
State gState = STOP;
Choice gCurAlgorithm = BubbleSort;

// -- Forward declaration of the main control function
bool do_control();

// -- Macro to encapsulate waiting until the next frame
//    and checking for button presses
#define WAITFRAME    { if (do_control()) return true; }

// -- Macro to encapsulate showing a frame
#define SHOWFRAME(b) { show(b); WAITFRAME; }

// -- Call a function that returns true when interrupted
#define CALL( F )    { bool do_stop = F; if (do_stop) return true; }

// ----------------------------------------------------------------------

// -- Put the number of LEDs here
#define NUM_LEDS 100

// -- An array of integers, one for each LED
int gValues[NUM_LEDS];

// -- Actual colors that are sent to the LEDs
CRGB gValueLEDs[NUM_LEDS];
CRGB gIndicatorLEDs[NUM_LEDS];
bool gWorkingOn[NUM_LEDS];

CRGBPalette16 gPal;

#define MAX_VAL 999
#define BLACK -1
#define WHITE -2

void convert_values()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        int val = gValues[i];
        if (val == BLACK)
            gValueLEDs[i] = CRGB::Black;
        else if (val == WHITE)
            gValueLEDs[i] = CRGB::White;
        else {
            int hue = map(val, 0, MAX_VAL, 0, 200);
            gValueLEDs[i] = ColorFromPalette(gPal, hue);
            if ( ! gWorkingOn[i])
                gValueLEDs[i].fadeToBlackBy(225);
        }
    }
}

void show(bool update_values)
{
    if (update_values)
        convert_values();
    FastLED.show();
}

void clear_indicators()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gIndicatorLEDs[i] = CRGB::Black;
    }
}

void clear_working_on()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gWorkingOn[i] = true;
    }
}

void working_on(int start, int finish)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gWorkingOn[i] = (i >= start and i <= finish);
    }
}

// ----------------------------------------------------------------------

#define MAX_SPEED 150
volatile int gSpeed = 15;

#define STEP_SPEED_CUTOFF 30
#define DISPLAY_SPEED_CUTOFF 90

volatile bool B_fell = false;
volatile bool B_must_rise = false;
volatile bool A_fell = false;
volatile bool A_must_rise = false;

// Interrupt on A changing state
void doEncoderA()
{
    int curA = digitalRead(SPEED_A_PIN);
    if (curA == LOW) {
        A_fell = true;
        if (B_fell and not B_must_rise) {
            // -- A just fell, B already fell
            gSpeed = gSpeed - 3;
            if (gSpeed < 1) gSpeed = 1;
            Serial.println(gSpeed);
            A_must_rise = true;
            B_must_rise = true;
        }
    } else {
        A_fell = false;
        A_must_rise = false;
    }
}

// Interrupt on B changing state, same as A above
void doEncoderB()
{
    int curB = digitalRead(SPEED_B_PIN);
    if (curB == LOW) {
        B_fell = true;
        if (A_fell and not A_must_rise) {
            // -- B just fell, A already fell
            gSpeed = gSpeed + 3;
            if (gSpeed > MAX_SPEED) gSpeed = MAX_SPEED;
            Serial.println(gSpeed);
            A_must_rise = true;
            B_must_rise = true;        }
    } else {
        B_fell = false;
        B_must_rise = false;
    }
}

// ----------------------------------------------------------------------

int gStep = 0;

TM1637Display gStepDisplay(STEP_CLK, STEP_DIO);

void display_step()
{
    if (gSpeed < STEP_SPEED_CUTOFF or 
        (gSpeed < DISPLAY_SPEED_CUTOFF and gStep % 10 == 0) or
        (gStep % 100 == 0)) {
        gStepDisplay.showNumberDec(gStep, true);
    }
}

void step()
{
    gStep++;
    display_step();
}

// ----------------------------------------------------------------------

void generate_random()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gValues[i] = BLACK;
    }
    
    for (int i = 0; i < NUM_LEDS; i++) {
        int val = random16( MAX_VAL );
        gValues[i] = val;
        show(true);
    }
}

void add_noise()
{
    clear_indicators();
    int which = random16(NUM_LEDS);
    int val = random16( MAX_VAL );
    gIndicatorLEDs[which] = CRGB::Orange;
    gValues[which] = val;
    show(true);
}

void reverse()
{
    for (int i = 0; i < NUM_LEDS/2; i++) {
        int temp = gValues[i];
        gValues[i] = gValues[(NUM_LEDS - i) - 1];
        gValues[(NUM_LEDS - i) - 1] = temp;
        show(true);
    }
}


// ----------------------------------------------------------------------

bool linear_search()
{
    int pos = 0;
    bool done = false;

    int r = random16(NUM_LEDS);
    int lookingfor = gValues[r];
    bool ordered = true;
    for (int i = 0; i < NUM_LEDS-1; i++) {
        if (gValues[i] > gValues[i+1]) {
            ordered = false;
            break;
        }
    }

    while (pos < NUM_LEDS and not done) {
        step();
        gIndicatorLEDs[pos] = CRGB::Yellow;
        SHOWFRAME(false);
        
        if (gValues[pos] == lookingfor) {
            done = true;
        } else {
            if (ordered and gValues[pos] > lookingfor) {
                done = true;
            } else {
                gIndicatorLEDs[pos] = CRGB::Black;
                pos++;
            }
        }
    }

    if (done) {
        if (pos < NUM_LEDS) {
            if (gValues[pos] == lookingfor)
                gIndicatorLEDs[pos] = CRGB::Green;
            else
                gIndicatorLEDs[pos] = CRGB::Red;
        }
    }
    show(false);
    return false;
}

bool binary_search()
{
    int low = 0;
    int high = NUM_LEDS - 1;
    bool found = false;
    int middle;

    int r = random16(NUM_LEDS);
    int lookingfor = gValues[r];
    bool ordered = true;
    for (int i = 0; i < NUM_LEDS-1; i++) {
        if (gValues[i] > gValues[i+1]) {
            ordered = false;
            break;
        }
    }
    
    //set_indicators(0, NUM_LEDS, CRGB::Aqua);

    while (low <= high and not found) {
        step();
        middle = (low + high) / 2;

        gIndicatorLEDs[middle] = CRGB::Yellow;
        SHOWFRAME(false);

        if (lookingfor == gValues[middle]) {
            found = true;
        } else {
            if (lookingfor < gValues[middle]) {
                for (int i = middle + 1; i <= high; i++) {
                    gIndicatorLEDs[i] = CRGB::Black;
                }
                high = middle - 1;
            } else {
                for (int i = low; i < middle; i++) {
                    gIndicatorLEDs[i] = CRGB::Black;
                }
                low = middle + 1;
            }
            SHOWFRAME(false);
        }
    }

    if (found)
        gIndicatorLEDs[middle] = CRGB::Green;
    else
        gIndicatorLEDs[middle] = CRGB::Red;
    show(false);
    return false;
}

bool swap(int i, int j)
{
    if (gSpeed < STEP_SPEED_CUTOFF) {
        gIndicatorLEDs[i] = CRGB::Red;
        gIndicatorLEDs[j] = CRGB::Red;
        SHOWFRAME(false);
    }

    gIndicatorLEDs[i] = CRGB::Yellow;
    gIndicatorLEDs[j] = CRGB::Yellow;
        
    int temp = gValues[i];
    gValues[i] = gValues[j];
    gValues[j] = temp;

    if (gSpeed < DISPLAY_SPEED_CUTOFF) {
        SHOWFRAME(true);
    } else {
        WAITFRAME;
    }

    gIndicatorLEDs[i] = CRGB::Black;
    gIndicatorLEDs[j] = CRGB::Black;

    return false;
}

bool dont_swap(int i, int j)
{
    if (gSpeed < DISPLAY_SPEED_CUTOFF) {
        gIndicatorLEDs[i] = CRGB::Yellow;
        gIndicatorLEDs[j] = CRGB::Yellow;
    
        SHOWFRAME(false);
        if (gSpeed < STEP_SPEED_CUTOFF) {
            WAITFRAME;
        }
        
        gIndicatorLEDs[i] = CRGB::Black;
        gIndicatorLEDs[j] = CRGB::Black;
    } else {
        WAITFRAME;
    }
    
    return false;
}

bool bubble_sort()
{
    for (int i = 0; i < NUM_LEDS - 1; i++) {
        bool did_swap = false;

        working_on(0, NUM_LEDS - i);
        show(true);
        
        for (int j = 0; j < (NUM_LEDS - i) - 1; j++) {
            step();
            if (gValues[j] > gValues[j + 1]) {
                CALL( swap(j, j + 1) );
                did_swap = true;
            } else
                CALL( dont_swap(j, j + 1) );
        }
        
        if ( ! did_swap ) break;
    }
    return false;
}

bool insertion_sort()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        working_on(0, i);
        step();

        int key = gValues[i];
        gIndicatorLEDs[i] = CRGB::Yellow;
        SHOWFRAME(true);
        
        int j = i;
        while (j > 0 && gValues[j - 1] > key) {
            step();
            gIndicatorLEDs[j] = CRGB::Black;
            gIndicatorLEDs[j-1] = CRGB::Yellow;
            gValues[j] = gValues[j-1];
            gValues[j-1] = key;
            if (gSpeed < DISPLAY_SPEED_CUTOFF) {
                SHOWFRAME(true);
            } else {
                WAITFRAME;
            }
            
            j = j - 1;
        }

        gIndicatorLEDs[j] = CRGB::Yellow;
        gValues[j] = key;
        SHOWFRAME(true);
        gIndicatorLEDs[j] = CRGB::Black;
    }

    return false;
}

int choose_pivot(int low, int high)
{
    if (high - low >= 2) {
        int a = gValues[low];
        int b = gValues[low+1];
        int c = gValues[low+2];
        
        if ((a < b && b < c) || (c < b && b < a)) 
            return low+1;
        else if ((b < a && a < c) || (c < a && a < b)) 
            return low;
        else
            return low+2;
    } else {
        return low;
    }
}

bool partition (int low, int high, int & pivot_pos)
{
    int pivot_start = choose_pivot(low, high);
    int left = low;
    int right = high;
    int pivot = gValues[pivot_start];

    //set_indicators(low, high, CRGB::Aqua);
    //SHOWFRAME(false);

    while ( left < right ) {
        // -- Move left while item < pivot
        while ( gValues[left] <= pivot ) {
            step();
            CALL( dont_swap(left, right) );
            left++;
        }

        // -- Move right while item > pivot
        while ( gValues[right] > pivot ) {
            step();
            CALL( dont_swap(left, right) );
            right--;
        }

        // -- Found values to swap
        if ( left < right ) {
            step();
            CALL( swap(left, right) );
        }
    }

    /* right is final position for the pivot */
    step();
    int pivot_end = right;
    CALL( swap(pivot_start, pivot_end) );

    pivot_pos = pivot_end;
    return false;
}

bool quick_sort_rec(int low, int high)
{
    if (low < high) {
        if (low == high - 1) {
            // -- Just two elements, either swap or don't
            step();
            if (gValues[low] > gValues[high]) {
                CALL( swap(low, high) );
            } else {
                CALL( dont_swap(low, high) );
            }
        } else {
            // -- Recursive case
            working_on(low, high);
            show(true);
            
            int pivotpos;
            CALL( partition(low, high, pivotpos) );

            SHOWFRAME(true);

            CALL( quick_sort_rec(low, pivotpos - 1) );
            CALL( quick_sort_rec(pivotpos + 1, high) );
        }
    }
    
    return false;
}

bool quick_sort()
{
    int low = 0;
    int high = NUM_LEDS - 1;
    CALL( quick_sort_rec(low, high));
    return false;
}

bool merge(int start, int mid, int end)
{
    int start2 = mid + 1;

    // If the direct merge is already sorted
    if (gValues[mid] <= gValues[start2]) {
        step();
        return false;
    }

    // Two pointers to maintain start
    // of both arrays to merge
    while (start <= mid && start2 <= end) {
        step();

        // If element 1 is in right place
        if (gValues[start] <= gValues[start2]) {
            CALL( dont_swap(start, start2) );
            start++;
        }
        else {
            int value = gValues[start2];
            int index = start2;

            // Shift all the elements between element 1
            // element 2, right by 1.
            while (index != start) {
                gValues[index] = gValues[index - 1];
                // step();
                // CALL( swap(index, index-1) );
                index--;
            }
            
            gIndicatorLEDs[start] = CRGB::Yellow;
            gIndicatorLEDs[start2] = CRGB::Yellow;
            
            gValues[start] = value;

            if (gSpeed < DISPLAY_SPEED_CUTOFF) {
                SHOWFRAME(true);
            } else {
                WAITFRAME;
            }
        
            gIndicatorLEDs[start] = CRGB::Black;
            gIndicatorLEDs[start2] = CRGB::Black;
            
            // Update all the pointers
            start++;
            mid++;
            start2++;
        }
    }

    return false;
}

/* l is for left index and r is right index of the
   sub-array of arr to be sorted */

bool merge_sort_rec(int l, int r)
{
    if (l < r) {

        working_on(l, r);
        show(true);

        // Same as (l + r) / 2, but avoids overflow
        // for large l and r
        int m = l + (r - l) / 2;

        // Sort first and second halves
        CALL( merge_sort_rec(l, m) );
        CALL( merge_sort_rec(m + 1, r) );

        working_on(l, r);
        show(true);

        CALL( merge(l, m, r) );
    }

    return false;
}

bool merge_sort()
{
    int low = 0;
    int high = NUM_LEDS - 1;
    CALL( merge_sort_rec(low, high));
    return false;
}

// -----------------------------------------------------------------

int g_Brightness = 50;

void setup()
{
    delay(500);
    Serial.begin(115200);

    gPal = RainbowColors_p;

    pinMode(STEP_CLK, OUTPUT);
    pinMode(STEP_DIO, OUTPUT);

    pinMode(SPEED_A_PIN, INPUT);
    pinMode(SPEED_B_PIN, INPUT);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(gButtons[i].pin, INPUT);
    }

    pinMode(VALUES_LED_PIN, OUTPUT);
    digitalWrite(VALUES_LED_PIN, LOW);
    pinMode(INDICATORS_LED_PIN, OUTPUT);
    digitalWrite(INDICATORS_LED_PIN, LOW);
    Serial.println();
    Serial.println("Setup...");
    FastLED.addLeds<WS2812, VALUES_LED_PIN, GRB>(gValueLEDs, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.addLeds<WS2812, INDICATORS_LED_PIN, GRB>(gIndicatorLEDs, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(g_Brightness);

    gStepDisplay.setBrightness(0x0a);
    gStepDisplay.showNumberDec(9999, true);

    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Red);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(400);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Blue);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    delay(400);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Green);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(400);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Black);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(400);

    clear_indicators();
    clear_working_on();
    generate_random();
    
    attachInterrupt(SPEED_A_PIN, doEncoderA, CHANGE);
    attachInterrupt(SPEED_B_PIN, doEncoderB, CHANGE);
    Serial.println("GO");
}

/** Program control
 *  Delay for t milliseconds while checking for buttons, possibly changing
 *  the current algorithm, updating the data, or pausing.
 *  
 *  Returning true means that we are switching algorithms. Returning false
 *  means we should just continue.
 */
 uint32_t gNext_frame = 0;
bool do_control()
{
    while (millis() < gNext_frame or gState == PAUSE) {
        Choice c = check_buttons();
        if (c != None) {
            
            // -- Start/Pause button
            if (c == StartPause) {
                switch (gState) {
                case PAUSE:
                    // -- Continue running
                    gState = RUN;
                    Serial.println("PAUSE --> RUN");
                    break;
                case STOP:
                    // -- Start running with a new algorithm
                    gState = RUN;
                    Serial.println("STOP --> RUN");
                    return true;
                    break;
                case RUN:
                    // -- Pause for up to 
                    gState = PAUSE;
                    Serial.println("RUN --> PAUSE");
                    break;
                default:
                    break;
                }
            }

            if (c == GenRandom) {
                generate_random();
            }

            if (c == GenNoise) {
                add_noise();
            }

            if (c == Reverse) {
                reverse();
            }

            if (is_algorithm(c)) {
                Serial.print("CHANGE to "); Serial.println(c);
                gState = STOP;
                gCurAlgorithm = c;
                gStep = 0;
                display_step();
                clear_indicators();
                clear_working_on();
                show(false);
                return true;
            }
        }
        delay(1);
    }

    delay(1);

    gNext_frame = millis() + (1000/gSpeed);
    return false;
}

void loop()
{
    if (do_control()) {
        bool change = false;
        if (gState == RUN) {
            gStep = 0;
            display_step();
            uint32_t start_time = millis();
            switch (gCurAlgorithm) {
                case LinearSearch:
                    change = linear_search();
                    break;
                case BinarySearch:
                    change = binary_search();
                    break;
                case Reserved:
                    break;
                case BubbleSort:
                    change = bubble_sort();
                    break;
                case InsertionSort:
                    change = insertion_sort();
                    break;
                case QuickSort:
                    change = quick_sort();
                    break;
                case MergeSort:
                    change = merge_sort();
                    break;
                case HeapSort:
                    break;
                case BitonicSort:
                    break;
                default:
                    break;
            }
            if ( ! change) {
                // -- This means we finished the algorithm successfully
                clear_indicators();
                clear_working_on();
                show(true);
                gState = STOP;
                if (gStep > 0) {
                    uint32_t end_time = millis();
                    uint32_t fr = (end_time - start_time) / gStep;
                    Serial.print("Millis per frame: "); Serial.println(fr);
                }
            }
        }
    }
}
