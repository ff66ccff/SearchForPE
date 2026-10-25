# -*- coding: utf-8 -*-
"""生成 design/light.html、design/dark.html、design/states.html。

浅色与深色页面由同一份 DOM 生成，只有 <html data-theme> 不同，
保证两套稿子的排版完全一致。改数据后重跑本脚本即可。
"""
import io, os, re

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- 数据（真实题库）
MAIN_ITEMS = [
    dict(kind="judge", ans="错", hits=["健美操"],
         stem="竞技健美操集体五人项目必须是男女运动员混合的。"),
    dict(kind="judge", ans="对", hits=["健美操"],
         stem="健身性健美操一般的练习时间为一个小时左右，在练习的要求上根据个体情况而变化，"
              "严格遵循“健康、安全”的原则，防止运动损伤的出现，在保证安全的基础上，达到锻炼身体的目的。"),
    dict(kind="single", ans="C", hits=[],
         stem="排舞比赛中自选串烧排舞是指在指定的排舞曲目范围内，任选三首排舞并按照规则进行排舞的成套动作，"
              "其时间长度（包括上下场时间）不得超过_____分钟。",
         choice="A．4\nB．5\nC．6\nD．7"),
    dict(kind="multi", ans="BC", hits=[],
         stem="以下哪几种现象属于场上队员的技术犯规_____ 。",
         choice="A．推人\nB．故意延缓比赛时间\nC．假摔以伪造一次犯规\nD．过分用肘"),
    dict(kind="judge", ans="对", hits=["健美操"],
         stem="20世纪80年代，健美操传入我国。"),
    dict(kind="single", ans="C", hits=["健美操"],
         stem="有氧踏板属于下列哪个大项中的项目？",
         choice="A  啦啦操\nB  体操\nC  健美操\nD  技巧"),
    dict(kind="single", ans="D", hits=["健美操"],
         stem="一套健美操的编排是要根据音乐的_____ 、情节的变化而变化。",
         choice="A．节奏\nB．力度\nC．速度\nD．节奏、速度"),
    dict(kind="multi", ans="BC", hits=[],
         stem="抢防守篮板球技术动作包括_____ 。",
         choice="A．直接冲抢\nB．挡人抢位\nC．判断并及时起跳\nD．快速跟进"),
    dict(kind="single", ans="A", hits=[],
         stem="羽毛球单打比赛中，发球抢攻战术是指___",
         choice="A. 以发网前球或平快球为主，限制对方进攻，迫使对方出高球。然后运用杀球和吊球攻击对方的弱点或空档。\n"
                "B. 先以快速而准确的落点控制对方场区四个角落，迫使对方前后左右地来回奔跑，"
                "当对方来不及回中心位置或失去身体平衡时，抓住空档和其弱点进行突击。\n"
                "C. 通过自身出球的节奏变化和良好的击球动作一致性，针对对方队员的某一个技术薄弱点，"
                "或击球后回中心位置较快的特点，重复地将球攻击到对方场上的某一区域。"),
    dict(kind="judge", ans="错", hits=["健美操"],
         stem="竞技健美操比赛中女运动员着一件套带有肉色袜的比赛服，服装上可以使用松散或附加的饰物来增加效果。"),
]

TYPE_TEXT = {"judge": "判断题", "single": "单选题", "multi": "多选题"}
TYPE_CLASS = {"judge": "judge", "single": "single", "multi": "multi"}

SPRITE = """<svg class="sprite" aria-hidden="true" focusable="false">
  <symbol id="i-search" viewBox="0 0 16 16"><g fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"><circle cx="6.9" cy="6.9" r="4.4"/><path d="M10.3 10.3 13.8 13.8"/></g></symbol>
  <symbol id="i-search-20" viewBox="0 0 20 20"><g fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"><circle cx="8.6" cy="8.6" r="5.5"/><path d="M12.9 12.9 17 17"/></g></symbol>
  <symbol id="i-moon" viewBox="0 0 18 18"><path d="M15.3 11.2A6.35 6.35 0 0 1 7.1 3a6.6 6.6 0 1 0 8.2 8.2Z" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/></symbol>
  <symbol id="i-sun" viewBox="0 0 18 18"><g fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"><circle cx="9" cy="9" r="3.1"/><path d="M9 2.2v1.7M9 14.1v1.7M2.2 9h1.7M14.1 9h1.7M4.2 4.2l1.2 1.2M12.6 12.6l1.2 1.2M13.8 4.2l-1.2 1.2M5.4 12.6l-1.2 1.2"/></g></symbol>
  <symbol id="i-x" viewBox="0 0 14 14"><path d="M3.6 3.6 10.4 10.4M10.4 3.6 3.6 10.4" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></symbol>
  <symbol id="i-check" viewBox="0 0 12 12"><path d="M2.5 6.4 4.9 8.8 9.6 3.5" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></symbol>
  <symbol id="i-cross" viewBox="0 0 12 12"><path d="M3.2 3.2 8.8 8.8M8.8 3.2 3.2 8.8" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></symbol>
  <symbol id="i-copy" viewBox="0 0 15 15"><g fill="none" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"><rect x="5.3" y="5.3" width="7.9" height="7.9" rx="1.8"/><path d="M9.7 3.1H3.6A1.2 1.2 0 0 0 2.4 4.3v6.1"/></g></symbol>
  <symbol id="i-check-15" viewBox="0 0 15 15"><path d="M2.9 7.7 6.1 10.9 12.1 4.1" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></symbol>
  <symbol id="i-arrow-up" viewBox="0 0 16 16"><path d="M8 12.9V3.4M3.7 7.7 8 3.4l4.3 4.3" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></symbol>
</svg>"""

OPT_RE = re.compile(r"^\s*([A-Da-d])\s*[\.．、]?\s*(.*)$")


def parse_options(choice):
    out = []
    for line in choice.split("\n"):
        line = re.sub(r"[ \t]+", " ", line).strip()
        if not line:
            continue
        m = OPT_RE.match(line)
        if m:
            out.append((m.group(1).upper() + "．", m.group(2).strip()))
        else:
            out.append(("", line))
    return out


def pill(item):
    if item["kind"] == "judge":
        if item["ans"] == "对":
            return '<span class="pill pill--ok"><svg class="ic-12"><use href="#i-check"/></svg>正确</span>'
        return '<span class="pill pill--no"><svg class="ic-12"><use href="#i-cross"/></svg>错误</span>'
    txt = "答案 " + "·".join(item["ans"])
    return '<span class="pill pill--ans">%s</span>' % txt


def item_html(item, idx, ind="      "):
    cls = "item"
    if item.get("hover"):
        cls += " is-hover"
    if item.get("copied"):
        cls += " is-copied"
    L = []
    L.append(ind + '<article class="%s">' % cls)
    L.append(ind + '  <div class="item-meta">')
    L.append(ind + '    <span class="idx">%02d</span>' % idx)
    L.append(ind + '    <span class="qtype"><i class="dot dot--%s"></i>%s</span>'
             % (TYPE_CLASS[item["kind"]], TYPE_TEXT[item["kind"]]))
    if item.get("hits"):
        chips = "".join('<span class="hit">%s</span>' % h for h in item["hits"][:3])
        L.append(ind + '    <span class="hits">%s</span>' % chips)
    L.append(ind + '    <span class="meta-right">')
    if item.get("copied"):
        L.append(ind + '      <span class="copied">已复制</span>')
    L.append(ind + '      ' + pill(item))
    if item.get("copied"):
        L.append(ind + '      <button class="copy-btn is-done" aria-label="已复制">'
                      '<svg class="ic-15"><use href="#i-check-15"/></svg></button>')
    else:
        L.append(ind + '      <button class="copy-btn" aria-label="复制">'
                      '<svg class="ic-15"><use href="#i-copy"/></svg></button>')
    L.append(ind + '    </span>')
    L.append(ind + '  </div>')
    L.append(ind + '  <p class="stem">%s</p>' % item["stem"])
    if item.get("choice"):
        L.append(ind + '  <div class="opts">')
        keys = set(c for c in item["ans"]) if item["kind"] == "multi" else set(item["ans"])
        for k, t in parse_options(item["choice"]):
            hot = " is-key" if k and k[0] in keys else ""
            L.append(ind + '    <div class="opt%s"><span class="opt-key">%s</span>'
                          '<span class="opt-text">%s</span></div>' % (hot, k, t))
        L.append(ind + '  </div>')
    L.append(ind + '</article>')
    return "\n".join(L)


def items_block(items, ind="      "):
    return "\n".join(item_html(it, i + 1, ind) for i, it in enumerate(items))


# ---------------------------------------------------------------- 尺寸核算
def item_metrics(it):
    stem_lines = len(it["stem"]) / 63.0
    n = max(1, int(-(-len(it["stem"]) // 63)))
    h = 18 + 22 + 10 + 25.5 * n + 18
    if it.get("choice"):
        k = len(parse_options(it["choice"]))
        h += 8 + (21.6 * k + 2 * (k - 1))
    return h


def page(theme, title, body):
    return """<!doctype html>
<html lang="zh-CN" data-theme="%s">
<head>
<meta charset="utf-8">
<title>%s</title>
<link rel="stylesheet" href="mock.css">
</head>
<body class="page">
%s
%s
</body>
</html>
""" % (theme, title, SPRITE, body)


MAIN_BODY = """<div class="app">
  <!-- 1. 搜索 + 主题 -->
  <div class="search">
    <svg class="search-ico"><use href="#i-search"/></svg>
    <span class="search-value">健美操</span>
    <button class="search-clear" aria-label="清除"><svg class="ic-14"><use href="#i-x"/></svg></button>
  </div>
  <button class="theme-btn" aria-label="切换主题"><svg class="ic-18"><use href="#i-%s"/></svg></button>

  <!-- 2. 筛选 -->
  <div class="filterbar">
    <div class="filterbar-inner">
      <div class="chips">
        <span class="chip is-active">全部<span class="num">6108</span></span>
        <span class="chip">判断题<span class="num">3167</span></span>
        <span class="chip">选择题<span class="num">2941</span></span>
      </div>
      <div class="toggle">
        <span class="toggle-label">检索选项</span>
        <span class="switch"><i class="knob"></i></span>
      </div>
    </div>
  </div>

  <!-- 3. 结果信息 -->
  <div class="result-meta">
    <span class="count"><b>239</b> 条结果</span>
    <span class="timing">0.45 ms</span>
  </div>

  <!-- 4. 结果面板（只有滚动，没有翻页） -->
  <section class="panel panel--abs">
    <div class="panel-scroll">
%s
    </div>
    <div class="scrollbar"><i class="scrollbar-thumb"></i></div>
  </section>

  <!-- 5. 回到顶部 -->
  <button class="fab" aria-label="回到顶部"><svg class="ic-16"><use href="#i-arrow-up"/></svg></button>
</div>"""


MEASURE_JS = """<script>
function r(el){const b=el.getBoundingClientRect();return [Math.round(b.x*10)/10,Math.round(b.y*10)/10,Math.round(b.width*10)/10,Math.round(b.height*10)/10].join(",");}
const out=[];
out.push("search="+r(document.querySelector(".search")));
out.push("theme="+r(document.querySelector(".theme-btn")));
out.push("filterbar="+r(document.querySelector(".filterbar")));
out.push("chip1="+r(document.querySelector(".chip")));
out.push("switch="+r(document.querySelector(".switch")));
out.push("meta="+r(document.querySelector(".result-meta")));
out.push("panel="+r(document.querySelector(".panel")));
out.push("scrollthumb="+r(document.querySelector(".scrollbar-thumb")));
out.push("fab="+r(document.querySelector(".fab")));
document.querySelectorAll(".item").forEach((el,i)=>{
  out.push("item"+(i+1)+"="+r(el)+" stemH="+Math.round(el.querySelector(".stem").getBoundingClientRect().height*10)/10
    +" stemLines="+Math.round(el.querySelector(".stem").getBoundingClientRect().height/25.5*100)/100);
});
document.title=out.join(" || ");
</script>"""


def main():
    items = items_block(MAIN_ITEMS)
    hover_items = items_block([MAIN_ITEMS[0], dict(MAIN_ITEMS[1], hover=True)] + MAIN_ITEMS[2:])

    light = page("light", "体育理论题库查询 · 浅色", MAIN_BODY % ("moon", hover_items))
    dark = page("dark", "体育理论题库查询 · 深色", MAIN_BODY % ("sun", hover_items))

    # ---- 状态演示页 ----
    s1 = [
        dict(MAIN_ITEMS[1], hover=True),
        dict(MAIN_ITEMS[2], copied=True),
    ]
    s1_html = "\n".join(item_html(it, i + 1, "        ") for i, it in enumerate(s1))
    s3_html = item_html(dict(MAIN_ITEMS[8]), 1, "        ")

    states_body = """<div class="sheet">
  <div class="sec">
    <div class="sec-label">条目交互态 · hover（复制按钮浮现） / 已复制（1.2 s 内）</div>
    <section class="panel panel--flow">
      <div class="panel-scroll" style="position: static; overflow: visible;">
%s
      </div>
    </section>
  </div>

  <div class="sec">
    <div class="sec-label">空状态 · 无命中结果</div>
    <section class="panel panel--flow panel--empty">
      <div class="empty">
        <svg class="empty-ico"><use href="#i-search-20"/></svg>
        <div class="empty-title">没有找到相关题目</div>
        <div class="empty-hint">试试更短的关键词，或切换筛选条件</div>
      </div>
    </section>
  </div>

  <div class="sec">
    <div class="sec-label">边界数据 · 真实题库中的超长选项（悬挂缩进、第二行与正文左对齐）</div>
    <section class="panel panel--flow">
      <div class="panel-scroll" style="position: static; overflow: visible;">
%s
      </div>
    </section>
  </div>
</div>""" % (s1_html, s3_html)

    states = page("light", "体育理论题库查询 · 状态演示", states_body)

    measure = light.replace("</body>", MEASURE_JS + "\n</body>")
    open(os.path.join(HERE, "_measure.html"), "w", encoding="utf-8").write(measure)

    open(os.path.join(HERE, "light.html"), "w", encoding="utf-8").write(light)
    open(os.path.join(HERE, "dark.html"), "w", encoding="utf-8").write(dark)
    open(os.path.join(HERE, "states.html"), "w", encoding="utf-8").write(states)

    # ---- 打印排版核算，供 layout.md 使用 ----
    total = 0
    for i, it in enumerate(MAIN_ITEMS):
        h = item_metrics(it)
        total += h + (1 if i else 0)
        print("item %02d  h=%.1f  %s" % (i + 1, h, it["stem"][:18]))
    print("visible-sum(01-04) =", sum(item_metrics(it) for it in MAIN_ITEMS[:4]) + 3)
    print("content-total =", total)
    print("panel-inner = 682  thumb =", round(682 * 682 / total, 1))
    print("cut-after-item04 =", 682 - (sum(item_metrics(it) for it in MAIN_ITEMS[:4]) + 3))


if __name__ == "__main__":
    main()
