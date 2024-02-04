type ColorCodes = {
    [key: string]: string;
  };

type ColorTableCode = {
    [key: number]: string;
  };


const DEFAULT_COLORS: ColorCodes = {
    "0": "0",      // Normal
    "1": "0;36",   // Roomname
    "2": "0;32",   // Roomobjs
    "3": "0;33",   // Roompeople
    "4": "0;31",   // Hityou
    "5": "0;32",   // Youhit
    "6": "0;33",   // Otherhit
    "7": "1;33",   // Critical
    "8": "1;33",   // Holler
    "9": "1;33",   // Shout
    "10": "0;33",  // Gossip
    "11": "0;36",  // Auction
    "12": "0;32",  // Congrat
    "13": "0;31",  // Tell
    "14": "0;36",  // Yousay
    "15": "0;37"   // Roomsay
  };
  
  const COLOR_MAP: ColorCodes = {
        "n": "0",
  
    "d": "0;30",
    "b": "0;34",
    "g": "0;32",
    "c": "0;36",
    "r": "0;31",
    "m": "0;35",
    "y": "0;33",
    "w": "0;37",
  
    "D": "1;30",
    "B": "1;34",
    "G": "1;32",
    "C": "1;36",
    "R": "1;31",
    "M": "1;35",
    "Y": "1;33",
    "W": "1;37",
  
    "0": "40",
    "1": "44",
    "2": "42",
    "3": "46",
    "4": "41",
    "5": "45",
    "6": "43",
    "7": "47",
  
    "l": "5",
    "u": "4",
    "o": "1",
    "e": "7"
  };
  
  const RANDOM_CODES = ["b", "g", "c", "r", "m", "y", "w", "B", "G", "C", "R", "M", "W", "Y"];
  
  const RE_COLOR = /@(n|d|D|b|B|g|G|c|C|r|R|m|M|y|Y|w|W|x|0|1|2|3|4|5|6|7|l|o|u|e|@|\[\d+\])/g;
  
  
export function circleToAnsi(entry: string, colors?: ColorCodes): string {
    const customColors: ColorCodes = {...DEFAULT_COLORS, ...(colors || {})};
  
    function replaceColor(match: string, code: string): string {
        if (code === "@") {
            return "@";
        } else if (code === "x") {
            const randomCode = RANDOM_CODES[Math.floor(Math.random() * RANDOM_CODES.length)];
            return `\x1b[${COLOR_MAP[randomCode]}m`;
        } else if (code.startsWith("[")) {
            const colorCode = code.slice(1, -1);
            if (colorCode in customColors) {
                return `\x1b[${customColors[colorCode]}m`;
            } else {
                return match;
            }
        } else {
            return `\x1b[${COLOR_MAP[code] || '0'}m`;
        }
    }
  
    return entry.replace(RE_COLOR, replaceColor);
  }
  
export const RavensGleaning = {
    html: function(str: string, mushLog: boolean = false) {
        function colorIndexToHtml(bold: boolean, color: number) {
            //console.log("Color for " + color);
            if(color < 8 && bold) {
                //console.log("\t is bold");
                color += 8;
            }
            //console.log("\t" + colorTable[color]);
            return colorTable[color];
        }
  
        function updateState(state: any, command: string) {
            if(!Object.keys(state).includes('foreground')) {
                state.foreground = 7;
            }
  
            if(!Object.keys(state).includes('background')) {
                state.background = 0;
            }
  
            //console.log('Command: ' + command);
            if(command.substr(-1) === "m") {
                var parts = command.substr(0, command.length - 1).split(";");
                for(var i = 0; i < parts.length; ++i) {
                    var num = parseInt(parts[i]);
                    //console.log(num);
                    // Reset
                    if(num === 0) {
                        state = { foreground: 7, background: 0 };
                    } else if(num === 1) {
                        state.bold = true;
                    } else if(num === 4) {
                        state.underscore = true;
                    } else if(num === 5) {
                        state.blink = true;
                    } else if(num === 7) {
                        state.reverse = true;
                        // 16 color FG
                    } else if(num >= 30 && num <= 37) {
                        //console.log("set fg to " + num);
                        state.foreground = num - 30;
                        // 16 color BG
                    } else if(num >= 40 && num <= 47) {
                        //console.log("set bg to " + num);
                        state.background = num - 40;
                        // Extended FG color
                    } else if(num === 38) {
                        i++;
                        // 256 color
                        if(parseInt(parts[i]) === 5) {
                            i++;
                            state.foreground = parseInt(parts[i]);
                            //console.log("set fg to " + state.foreground);
                        }
                        // Extended BG color
                    } else if(num === 48) {
                        i++;
                        // 256 color
                        if(parseInt(parts[i]) === 5) {
                            i++;
                            state.background = parseInt(parts[i]);
                            //console.log("set bg to " + state.background);
                        }
                    }
                }
            }
  
            //console.log(state);
            return state;
        }
  
        function isStateReset(state: any) {
            return !state.bold &&
                !state.underscore &&
                !state.blink &&
                state.foreground === 7 &&
                state.background === 0;
        }
  
        function htmlForState(state: any) {
            var ret = "";
            ret += '<span style="';
            if(state.bold) {
                ret += "font-weight:bold;";
            }
            if(state.underscore) {
                ret += "text-decoration:underline;";
            }
            if(state.blink) {
                ret += "text-decoration:blink;";
            }
  
            var fg = colorIndexToHtml(state.bold, state.foreground);
            var bg = colorIndexToHtml(false, state.background);
  
            if(state.reverse) {
                ret += "color:" + bg + ";";
                ret += "background-color:" + fg + ";";
            } else {
                ret += "color:" + fg + ";";
                ret += "background-color:" + bg + ";";
            }
            ret += '">';
  
            return ret;
        }
  
        const colorTable: ColorTableCode = {
            0: "#000000",
            1: "#800000",
            2: "#008000",
            3: "#808000",
            4: "#000080",
            5: "#800080",
            6: "#008080",
            7: "#c0c0c0",
            8: "#808080",
            9: "#ff0000",
            10: "#00ff00",
            11: "#ffff00",
            12: "#0000ff",
            13: "#ff00ff",
            14: "#00ffff",
            15: "#ffffff",
            16: "#000000",
            17: "#00005f",
            18: "#000087",
            19: "#0000af",
            20: "#0000d7",
            21: "#0000ff",
            22: "#005f00",
            23: "#005f5f",
            24: "#005f87",
            25: "#005faf",
            26: "#005fd7",
            27: "#005fff",
            28: "#008700",
            29: "#00875f",
            30: "#008787",
            31: "#0087af",
            32: "#0087d7",
            33: "#0087ff",
            34: "#00af00",
            35: "#00af5f",
            36: "#00af87",
            37: "#00afaf",
            38: "#00afd7",
            39: "#00afff",
            40: "#00d700",
            41: "#00d75f",
            42: "#00d787",
            43: "#00d7af",
            44: "#00d7d7",
            45: "#00d7ff",
            46: "#00ff00",
            47: "#00ff5f",
            48: "#00ff87",
            49: "#00ffaf",
            50: "#00ffd7",
            51: "#00ffff",
            52: "#5f0000",
            53: "#5f005f",
            54: "#5f0087",
            55: "#5f00af",
            56: "#5f00d7",
            57: "#5f00ff",
            58: "#5f5f00",
            59: "#5f5f5f",
            60: "#5f5f87",
            61: "#5f5faf",
            62: "#5f5fd7",
            63: "#5f5fff",
            64: "#5f8700",
            65: "#5f875f",
            66: "#5f8787",
            67: "#5f87af",
            68: "#5f87d7",
            69: "#5f87ff",
            70: "#5faf00",
            71: "#5faf5f",
            72: "#5faf87",
            73: "#5fafaf",
            74: "#5fafd7",
            75: "#5fafff",
            76: "#5fd700",
            77: "#5fd75f",
            78: "#5fd787",
            79: "#5fd7af",
            80: "#5fd7d7",
            81: "#5fd7ff",
            82: "#5fff00",
            83: "#5fff5f",
            84: "#5fff87",
            85: "#5fffaf",
            86: "#5fffd7",
            87: "#5fffff",
            88: "#870000",
            89: "#87005f",
            90: "#870087",
            91: "#8700af",
            92: "#8700d7",
            93: "#8700ff",
            94: "#875f00",
            95: "#875f5f",
            96: "#875f87",
            97: "#875faf",
            98: "#875fd7",
            99: "#875fff",
            100: "#878700",
            101: "#87875f",
            102: "#878787",
            103: "#8787af",
            104: "#8787d7",
            105: "#8787ff",
            106: "#87af00",
            107: "#87af5f",
            108: "#87af87",
            109: "#87afaf",
            110: "#87afd7",
            111: "#87afff",
            112: "#87d700",
            113: "#87d75f",
            114: "#87d787",
            115: "#87d7af",
            116: "#87d7d7",
            117: "#87d7ff",
            118: "#87ff00",
            119: "#87ff5f",
            120: "#87ff87",
            121: "#87ffaf",
            122: "#87ffd7",
            123: "#87ffff",
            124: "#af0000",
            125: "#af005f",
            126: "#af0087",
            127: "#af00af",
            128: "#af00d7",
            129: "#af00ff",
            130: "#af5f00",
            131: "#af5f5f",
            132: "#af5f87",
            133: "#af5faf",
            134: "#af5fd7",
            135: "#af5fff",
            136: "#af8700",
            137: "#af875f",
            138: "#af8787",
            139: "#af87af",
            140: "#af87d7",
            141: "#af87ff",
            142: "#afaf00",
            143: "#afaf5f",
            144: "#afaf87",
            145: "#afafaf",
            146: "#afafd7",
            147: "#afafff",
            148: "#afd700",
            149: "#afd75f",
            150: "#afd787",
            151: "#afd7af",
            152: "#afd7d7",
            153: "#afd7ff",
            154: "#afff00",
            155: "#afff5f",
            156: "#afff87",
            157: "#afffaf",
            158: "#afffd7",
            159: "#afffff",
            160: "#d70000",
            161: "#d7005f",
            162: "#d70087",
            163: "#d700af",
            164: "#d700d7",
            165: "#d700ff",
            166: "#d75f00",
            167: "#d75f5f",
            168: "#d75f87",
            169: "#d75faf",
            170: "#d75fd7",
            171: "#d75fff",
            172: "#d78700",
            173: "#d7875f",
            174: "#d78787",
            175: "#d787af",
            176: "#d787d7",
            177: "#d787ff",
            178: "#d7af00",
            179: "#d7af5f",
            180: "#d7af87",
            181: "#d7afaf",
            182: "#d7afd7",
            183: "#d7afff",
            184: "#d7d700",
            185: "#d7d75f",
            186: "#d7d787",
            187: "#d7d7af",
            188: "#d7d7d7",
            189: "#d7d7ff",
            190: "#d7ff00",
            191: "#d7ff5f",
            192: "#d7ff87",
            193: "#d7ffaf",
            194: "#d7ffd7",
            195: "#d7ffff",
            196: "#ff0000",
            197: "#ff005f",
            198: "#ff0087",
            199: "#ff00af",
            200: "#ff00d7",
            201: "#ff00ff",
            202: "#ff5f00",
            203: "#ff5f5f",
            204: "#ff5f87",
            205: "#ff5faf",
            206: "#ff5fd7",
            207: "#ff5fff",
            208: "#ff8700",
            209: "#ff875f",
            210: "#ff8787",
            211: "#ff87af",
            212: "#ff87d7",
            213: "#ff87ff",
            214: "#ffaf00",
            215: "#ffaf5f",
            216: "#ffaf87",
            217: "#ffafaf",
            218: "#ffafd7",
            219: "#ffafff",
            220: "#ffd700",
            221: "#ffd75f",
            222: "#ffd787",
            223: "#ffd7af",
            224: "#ffd7d7",
            225: "#ffd7ff",
            226: "#ffff00",
            227: "#ffff5f",
            228: "#ffff87",
            229: "#ffffaf",
            230: "#ffffd7",
            231: "#ffffff",
            232: "#080808",
            233: "#121212",
            234: "#1c1c1c",
            235: "#262626",
            236: "#303030",
            237: "#3a3a3a",
            238: "#444444",
            239: "#4e4e4e",
            240: "#585858",
            241: "#606060",
            242: "#666666",
            243: "#767676",
            244: "#808080",
            245: "#8a8a8a",
            246: "#949494",
            247: "#9e9e9e",
            248: "#a8a8a8",
            249: "#b2b2b2",
            250: "#bcbcbc",
            251: "#c6c6c6",
            252: "#d0d0d0",
            253: "#dadada",
            254: "#e4e4e4",
            255: "#eeeeee"
        }
  
        var buf = str;
        var len = str.length;
        var ret = "";
        var offset = 0;
        var state = {};
        state = updateState(state, '');
        if(!mushLog) {
            ret = htmlForState(state);
        }
        do { try {
            // Read next byte
            var byte = buf.codePointAt(offset);
  
            // If we see ESC, get to work
            if(byte === "\u001b".codePointAt(0)) {
                // If next char is [, this is a sequence we care about
                if(offset + 1 < len && buf.codePointAt(offset + 1) === "[".codePointAt(0)) {
                    // Jump past the CSI
                    offset += 2;
                    var command = "";

                    // Read ahead until we hit something that isn't a
                    // number or a semicolon
                    do {
                        var char = String.fromCodePoint(buf.codePointAt(offset) as number);
                        command += char;
                    } while (++offset < len && char.match(/[0-9;]/));

                    var fromReset = isStateReset(state);
                    // Process the command
                    state = updateState(state, command);
                    var toReset = isStateReset(state);
                    if(!fromReset || !mushLog) {
                        ret += "</span>";
                    }
                    if(!toReset || !mushLog) {
                        ret += htmlForState(state);
                    }
                    continue;
                }
            } else if (byte === "&".codePointAt(0)) {
                ret += "&amp;";
                ++offset;
                continue;
            } else if (byte === "<".codePointAt(0)) {
                ret  += "&lt;";
                ++offset;
                continue;
            } else if (byte === ">".codePointAt(0)) {
                ret += "&gt;";
                ++offset;
                continue;
            } else if (byte === '"'.codePointAt(0)) {
                ret += "&quot;";
                ++offset;
                continue;
            }
  
            // Pass through
            offset++;
            ret += String.fromCodePoint(byte ?? 0);
        } catch(e) { break }
        } while(offset < len);
        var isReset = isStateReset(state);
        if(!isReset || !mushLog) {
            ret += "</span>";
        }
        return ret.toString();
    }
  };