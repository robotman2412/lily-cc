
// RegExp helper.
RegExp.prototype.findAll = function(str) {
    var match = [];
    var match1;
    do {
        match1 = this.exec(str);
        if (match1) match = match.concat(match1);
    } while (match1);
    return match;
}


// Tree based on first keyword.
var tree = {};
isaDef.instructions.forEach(e => {
    var match = /^\w+/i.exec(e.name);
    var index = match[0].toLowerCase();
    if (!tree[index]) tree[index] = [];
    tree[index] = tree[index].concat(e);
});


// Keywords.
keywords = [];
for (var i in tree) {
    keywords = keywords.concat(i);
}
isaDef.instructions.forEach(e => {
    var match = /\w+/gi.findAll(e.name);
    for (var i in match) {
        var lMatch = match[i].toLowerCase();
        if (keywords.indexOf(lMatch) == -1) keywords = keywords.concat(lMatch);
    }
});


// Keyword arrays.
var keyword_str  = "    ";
var keyword_enum = "    ";
var index = 0;
for (var i in keywords) {
    var keyw = keywords[i];
    if (index == 4) {
        index = 0;
        keyword_str  += '\n    ';
        keyword_enum += '\n    ';
    }
    keyword_str  += `"${keyw}", `;
    keyword_enum += `R3_KEYW_${keyw.toUpperCase()}, `;
    index ++;
}
console.log(keyword_str);
console.log(keyword_enum);


// Mode look up table.
var modelut = {
    "A":     "REG_A",
    "X":     "REG_X",
    "Y":     "REG_Y",
    "F":     "REG_F",
    "STL":   "REG_STL",
    "STH":   "REG_STH",
    "%":     "IMM",
    "[%]":   "MEM",
    "X[%]":  "MEM_X",
    "Y[%]":  "MEM_Y",
    "(%)":   "PTR",
    "X(%)":  "PTR_X",
    "(%)Y":  "PTR_Y",
    "X(%)Y": "PTR_XY",
};
var res=`r3_iasm_modes_t r3_insn_lut[${Object.keys(tree).length}] = {\n`;
for (var i in tree) {
    res += `\t{ // ${i}\n`;
    res += `\t\t.num = ${tree[i].length}, .modes = (r3_iasm_mode_t[]) {\n`;
    tree[i].forEach(e => {
        var e1 = e.name.match(/^\w+ (.+)$/i);
        var e2 = e.name.match(/^\w+ (.+?), (.+)$/i);
        var n_words = e.args.length == 1 ? e.args[0].type.bits / 8 : 0;
        if (e2) {
            // Two operands
            var mode0 = modelut[e2[1]];
            var mode1 = modelut[e2[2]];
            res += `\t\t\t{ .n_args=2, .opcode=0x${e.hex}, .n_words=${n_words}, .arg_modes={A_${mode0}, A_${mode1}}},\n`;
        } else if (e1) {
            // One operand.
            var mode = modelut[e1[1]];
            res += `\t\t\t{ .n_args=1, .opcode=0x${e.hex}, .n_words=${n_words}, .arg_modes={A_${mode}}},\n`;
        } else {
            // No operands.
            res += `\t\t\t{ .n_args=0, .opcode=0x${e.hex}, .n_words=0},\n`;
        }
    });
    res += `\t\t}\n`;
    res += `\t},\n`;
}
res += '};';
console.log(res);


for (var i in tree) {
    console.log(`${i}:`)
    tree[i].forEach(e => {
        console.log('  '+e.name.replaceAll(/\[%\]/g, '[adr]').replaceAll(/\(%\)/g, '(ptr)').replaceAll('%', 'imm'));
    });
}
