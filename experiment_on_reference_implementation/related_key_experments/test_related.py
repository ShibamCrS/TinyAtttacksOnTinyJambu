from termcolor import colored
import math

linear_taps = [91, 47, 0]
non_linear_taps = [85, 70]

def print_bit(value):
    if value:
        return "1"
    else:
        return "."
    

def print_state_key(state, key):
    s = ""
    for i, key_bit in enumerate(key):
        key_bit = print_bit(key_bit)
        if i == 0:
            s += colored(f"{key_bit}", "green")
        else:
            s += f"{key_bit}"
    
    s += "\n"
    for i, state_bit in enumerate(reversed(state)):
        state_bit = print_bit(state_bit)
        i = 127 - i
        if i in linear_taps:
            s += colored(f"{state_bit}", "green")
        elif i in non_linear_taps:
            s += colored(f"{state_bit}", "red")
        else:
            s += f"{state_bit}"
    s += "\n"
    
    return s

def update(state, key):
    feedback_value = key[0]
    probability = 1
    for tap in linear_taps:
        feedback_value ^= state[tap]

    for tap in non_linear_taps:
        if state[tap]:
            probability = 0.5
    return state[1:] + [feedback_value], key[1:] + key[0:1], probability

def test_related_key_128():
    state = [0]*128
    key = [0]*128
    
    offset = 0

    key[0 + offset] = 1
    key[37 + offset] = 1
    key[81 + offset] = 1
    #key[128 + offset] = 1

    print(print_state_key(state, key))

    rounds = 640
    probability = 1
    for i in range(rounds):
        state, key, round_probability = update(state, key)
        # print(f"ROUND {i}")
        # print(print_state_key(state, key))

        probability *= round_probability
    probability = math.log2(probability)
    print(print_state_key(state, key))
    print(f"Probability (log2): {probability}")

def test_related_key_192():
    state = [0]*128
    key = [0]*192
    
    offset = 63

    key[0 + offset] = 1
    key[37 + offset] = 1
    key[81 + offset] = 1
    key[128 + offset] = 1

    print(print_state_key(state, key))

    rounds = 192
    probability = 1
    for i in range(rounds):
        state, key, round_probability = update(state, key)
        # if i in {0,1,36,37,42,43,57,58,80,81,127,128}:
        if (i-offset) in {0,1,36,37,42,43,57,58,80,81,127,128}:
            print(f"ROUND {i}")
            for j in range(128):
                if(state[j] == 1):
                    print("state active: ",j)
            print(print_state_key(state, key))

        probability *= round_probability
    probability = math.log2(probability)
    print(print_state_key(state, key))
    print(f"Probability (log2): {probability}")

def test_related_key_256():
    state = [0]*128
    key = [0]*256
    offset = 127
    key[0 + offset] = 1
    key[37 + offset] = 1
    key[81 + offset] = 1
    key[128 + offset] = 1


    print(print_state_key(state, key))

    rounds = 256 # 1280*2 + 640*3
    probability = 1
    for i in range(rounds):
        state, key, round_probability = update(state, key)
        # if i in {0,1,36,37,42,43,57,58,80,81,127,128}:
        if (i-127) in {0,1,36,37,42,43,57,58,80,81,127,128}:
            print(f"ROUND {i}")
            for j in range(128):
                if(state[j] == 1):
                    print("state active: ",j)
            print(print_state_key(state, key))

        probability *= round_probability

    probability = math.log2(probability)
    print(print_state_key(state, key))
    print(f"Probability (log2): {probability}")

# test_related_key_256()
test_related_key_192()
# test_related_key_128()
