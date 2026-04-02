import random
from rich.text import Text
from rich.ansi import AnsiDecoder

def _circle_to_ansi(text: str, custom_colors: dict[int, str] | None = None, rng: random.Random | None = None) -> str:
    if custom_colors is None:
        custom_colors = {}
    if rng is None:
        rng = random.Random(42)
    
    RANDOM_COLORS = "bgcrmywBGCRMWY"
    
    result = []
    i = 0
    while i < len(text):
        if text[i] != '@':
            result.append(text[i])
            i += 1
            continue
        
        i += 1
        if i >= len(text):
            result.append('@')
            break
        
        code = text[i]
        
        if code == '@':
            result.append('@')
            i += 1
            continue
        
        if code == 'n':
            result.append('\033[0m')
            i += 1
            continue
        
        if code in 'dDbBgGcCrRmMyYwW0':
            is_upper = code.isupper()
            code_lower = code.lower()
            
            color_map = {
                'd': '0', 'b': '1', 'g': '2', 'c': '3',
                'r': '4', 'm': '5', 'y': '6', 'w': '7'
            }
            fg_code = color_map.get(code_lower, '0')
            
            if code in '01234567':
                result.append(f'\033[4{fg_code}m')
            else:
                prefix = '1;' if is_upper else ''
                result.append(f'\033[{prefix}3{fg_code}m')
            i += 1
            continue
        
        attr_map = {
            'l': '5',  # blink
            'o': '1',  # bold
            'u': '4',  # underline
            'e': '7',  # reverse
            'L': '5',  # uppercase L = blink
            'O': '1',  # uppercase O = bold
            'U': '4',  # uppercase U = underline
            'E': '7',  # uppercase E = reverse
        }
        if code in attr_map:
            result.append(f'\033[{attr_map[code]}m')
            i += 1
            continue
        
        if code == 'x':
            random_code = rng.choice(RANDOM_COLORS)
            is_upper = random_code.isupper()
            code_lower = random_code.lower()
            color_map = {
                'd': '0', 'b': '1', 'g': '2', 'c': '3',
                'r': '4', 'm': '5', 'y': '6', 'w': '7'
            }
            fg_code = color_map.get(code_lower, '0')
            prefix = '1;' if is_upper else ''
            result.append(f'\033[{prefix}3{fg_code}m')
            i += 1
            continue
        
        if code == '[':
            start = i + 1
            end = start
            while end < len(text) and text[end].isdigit():
                end += 1
            if start == end:
                i += 1
                continue
            if end < len(text) and text[end] == ']':
                end += 1
            color_index = int(text[start:end-1] if end > start else text[start:end])
            color_name = custom_colors.get(color_index, 'black')
            ansi_code = _color_name_to_ansi(color_name)
            result.append(f'\033[{ansi_code}m')
            i = end
            continue
        
        if code == '<':
            start = i + 1
            end = start
            while end < len(text) and text[end] != '>':
                end += 1
            if end >= len(text) or text[end] != '>':
                i += 1
                continue
            sub = text[start:end]
            i = end + 1
            
            sub = sub.strip()
            if not sub:
                continue
            
            if ',' in sub:
                parts = sub.split(',')
                if len(parts) == 3:
                    try:
                        r, g, b = int(parts[0]), int(parts[1]), int(parts[2])
                        if 0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255:
                            result.append(f'\033[38;2;{r};{g};{b}m')
                            continue
                    except ValueError:
                        pass
                continue
            
            try:
                index = int(sub)
                if 0 <= index <= 255:
                    if index < 16:
                        result.append(f'\033[38;5;{index}m')
                    else:
                        result.append(f'\033[38;5;{index}m')
                    continue
            except ValueError:
                pass
            
            result.append(f'\033[{_color_name_to_ansi(sub.replace(" ", "_"))}m')
            continue
        
        i += 1
    
    return ''.join(result)

def _color_name_to_ansi(name: str) -> str:
    name = name.lower()
    mapping = {
        'black': '30', 'red': '31', 'green': '32', 'yellow': '33',
        'blue': '34', 'magenta': '35', 'cyan': '36', 'white': '37',
        'default': '39',
    }
    return mapping.get(name, '30')

def convert_color_string(text: str, custom_colors: dict[int, str] | None = None, seed: int | None = None) -> Text:
    rng = random.Random(seed) if seed is not None else None
    ansi = _circle_to_ansi(text, custom_colors, rng)
    decoder = AnsiDecoder()
    result = Text()
    for chunk in decoder.decode(ansi):
        result.append(chunk)
    return result