#include <FastLED.h>

// Total number of pins:
// Input:
//    12 buttons
//     2 for rotary encoder
// Output:
//     2 for LED strips
//     2 for 7 seg display

#define VALUES_LED_PIN 27
#define INDICATORS_LED_PIN 13

// -- Put the number of LEDs here
#define NUM_LEDS 100

// -- An array of integers, one for each LED
int gValues[NUM_LEDS];

// -- Actual colors that are sent to the LEDs
CRGB gValueLEDs[NUM_LEDS];
CRGB gIndicatorLEDs[NUM_LEDS];

// -- Algorithms
enum Function {
    LinearSearch, OrderedLinearSearch, BinarySearch, 
    BubbleSort, InsertionSort, QuickSort, MergeSort, HeapSort,
    GenRandom, GenSorted, AddNoise,
    Start, Stop 
};

// -- Program state
enum State { RUN, PAUSE, STOP };

State gState = STOP;

// ----------------------------------------------------------------------

struct Button
{
    int pin;
    Function func;
    int sense;
    bool pushed;
};

#define NUM_BUTTONS 12
Button gButtons[NUM_BUTTONS] = {
    { 15, LinearSearch, false },
    { 16, BinarySearch, false },
    { 17, BubbleSort, false },
    { 18, InsertionSort, false },
    { 25, QuickSort, false },
    { 26, MergeSort, false },
    { 27, HeapSort, false },
    { 28, GenRandom, false },
    { 29, GenSorted, false },
    { 30, AddNoise, false },
    { 12, Start, false },
    { 13, Stop, false},
};

void check_buttons()
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(gButtons[i].pin) == HIGH) {
            gButtons[i].sense++;
            if (gButtons[i].sense == 2) {
                gButtons[i].pushed = true;
            }
        } else {
            gButtons[i].sense = 0;
        }
    }
}


// ----------------------------------------------------------------------

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
            int hue = map(val, 0, MAX_VAL, 0, 220);
            gValueLEDs[i] = ColorFromPalette(gPal, hue);
        }
    }
}

void show(bool update_values = false)
{
    if (update_values)
        convert_values();
    FastLED.show();
}

// ----------------------------------------------------------------------

int gSpeed = 5;
int gDelay = 200;

void update_speed()
{
    // -- Read encoder
    // -- Send speed to display
    gDelay = 1000/gSpeed;
    
}

void dopause()
{
    // -- Check the buttons, too
    delay(gSpeed);
}

// ----------------------------------------------------------------------

int gStep = 0;

void show_step()
{
    // Send step number to display
}

void step()
{
    gStep++;
    show_step();
}

// ----------------------------------------------------------------------

void generate_random()
{
    for (int i = 0; i < NUM_LEDS; i++) {
        int val = random16( MAX_VAL );
        gValues[i] = val;
        show(true);
        dopause();
    }
}

void generate_ordered()
{
    int temp[NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++) {
        temp[i] = random16( MAX_VAL );
    }
    // Sort
}

// ----------------------------------------------------------------------

void linear_search(int lookingfor, bool ordered)
{
    int pos = 0;
    bool done = false;
    
    while (pos < NUM_LEDS and not done) {
        step();
        if (gValues[pos] == lookingfor) {
            done = true;
        } else {
            if (ordered and gValues[pos] > lookingfor) {
                done = true;
            } else {
                gIndicatorLEDs[pos] = CRGB::Yellow;
            }
        }
        pos++;

        show();
        dopause();
    }

    if (done) {
        if (pos < 100) {
            if (gValues[pos] == lookingfor)
                gIndicatorLEDs[pos] = CRGB::Green;
            else 
                gIndicatorLEDs[pos] = CRGB::Red;
        }
    }
    show();
}


void binary_search(int lookingfor)
{
    int low = 0;
    int high = NUM_LEDS - 1;
    bool found = false;
    int middle;
    
    while (low < high and not found) {
        step();
        middle = (low + high) / 2;
        
        gIndicatorLEDs[low]  = CRGB::Aqua;
        gIndicatorLEDs[high] = CRGB::Aqua;
        gIndicatorLEDs[middle] += CRGB::Yellow;
        show();
        dopause();
    
        if (lookingfor == gValues[middle]) {
            found = true;
        } else {
            if (lookingfor < gValues[middle]) {
                gIndicatorLEDs[high] = CRGB::Black;
                high = middle - 1;
            } else {
                gIndicatorLEDs[low] = CRGB::Black;
                low = middle + 1;
            }
            
        }
    }

    if (found)
        gIndicatorLEDs[middle] = CRGB::Green;
    else
        gIndicatorLEDs[middle] = CRGB::Red;
    show();
}

void swap(int i, int j)
{
    gIndicatorLEDs[i] = CRGB::Red;
    gIndicatorLEDs[j] = CRGB::Red;
    show();
    dopause();
    
    int temp = gValues[i];
    gValues[i] = gValues[j];
    gValues[j] = temp;
    gIndicatorLEDs[i] = CRGB::Green;
    gIndicatorLEDs[j] = CRGB::Green;
    show(true);
    dopause();
    
    gIndicatorLEDs[i] = CRGB::Black;
    gIndicatorLEDs[j] = CRGB::Black;
    show();
}

void dont_swap(int i, int j)
{
    gIndicatorLEDs[i] = CRGB::Green;
    gIndicatorLEDs[j] = CRGB::Green;
    show();
    dopause();
    dopause();
    
    gIndicatorLEDs[i] = CRGB::Black;
    gIndicatorLEDs[j] = CRGB::Black;
    show();
}

void bubble_sort()
{
    for (int i = 0; i < NUM_LEDS - 1; i++) {
        for (int j = 0; j < (NUM_LEDS - i) - 1; j++) {
            step();
            if (gValues[j] > gValues[j+1])
                swap(j, j+1);
            else
                dont_swap(j, j+1);
        }
    }
}

int partition (int low, int high) 
{
    int pivot_start = low;
    int left = low;
    int right = high;
    int pivot = gValues[pivot_start];

    gIndicatorLEDs[low]  = CRGB::Aqua;
    gIndicatorLEDs[high] = CRGB::Aqua;
    show();
    dopause();

    while ( left < right ) {
        // -- Move left while item < pivot
        while( gValues[left] <= pivot ) {
            step();
            dont_swap(left, right);
            left++;
        }
        
        // -- Move right while item > pivot
        while( gValues[right] > pivot ) {
            step();
            dont_swap(left, right);
            right--;
        }

        // -- Found values to swap
        if ( left < right ) {
            step();
            swap(left, right);
        }
    }
    
    /* right is final position for the pivot */
    int pivot_end = right;
    swap(pivot_start, pivot_end);

    return pivot_end;
} 

void quickSort(int low, int high) 
{
    if (low < high) {
        if (low == high-1) {
            // -- Just two elements, either swap or don't
            step();
            if (gValues[low] > gValues[high])
                swap(low, high);
            else
                dont_swap(low, high);
        } else {
            // -- Recursive case
            int pivotpos = partition(low, high);
  
            quickSort(low, pivotpos - 1);
            quickSort(pivotpos + 1, high);
        }
    }
} 


void merge(int arr[], int start, int mid, int end) 
{ 
    int start2 = mid + 1; 
  
    // If the direct merge is already sorted 
    if (arr[mid] <= arr[start2]) { 
        return; 
    } 
  
    // Two pointers to maintain start 
    // of both arrays to merge 
    while (start <= mid && start2 <= end) { 
  
        // If element 1 is in right place 
        if (arr[start] <= arr[start2]) { 
            start++; 
        } 
        else { 
            int value = arr[start2]; 
            int index = start2; 
  
            // Shift all the elements between element 1 
            // element 2, right by 1. 
            while (index != start) { 
                arr[index] = arr[index - 1]; 
                index--; 
            } 
            arr[start] = value; 
  
            // Update all the pointers 
            start++; 
            mid++; 
            start2++; 
        } 
    } 
} 
  
/* l is for left index and r is right index of the  
   sub-array of arr to be sorted */
   
void mergeSort(int arr[], int l, int r) 
{ 
    if (l < r) { 
  
        // Same as (l + r) / 2, but avoids overflow 
        // for large l and r 
        int m = l + (r - l) / 2; 
  
        // Sort first and second halves 
        mergeSort(arr, l, m); 
        mergeSort(arr, m + 1, r); 
  
        merge(arr, l, m, r); 
    } 
}

// -----------------------------------------------------------------

int g_Brightness = 50;

void setup() 
{
    delay(500);
    Serial.begin(115200);

    gPal = RainbowColors_p;
    
    pinMode(VALUES_LED_PIN, OUTPUT);
    digitalWrite(VALUES_LED_PIN, LOW);
    pinMode(INDICATORS_LED_PIN, OUTPUT);
    digitalWrite(INDICATORS_LED_PIN, LOW);
    Serial.println();
    Serial.println("Setup...");
    FastLED.addLeds<WS2812, VALUES_LED_PIN, GRB>(gValueLEDs, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.addLeds<WS2812, INDICATORS_LED_PIN, GRB>(gIndicatorLEDs, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(g_Brightness);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Red);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(1000);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Green);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(1000);
    fill_solid(gValueLEDs, NUM_LEDS, CRGB::Black);
    fill_solid(gIndicatorLEDs, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void loop()
{
    
}
