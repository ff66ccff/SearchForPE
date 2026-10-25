# 设计令牌（Design Tokens）

所有数值与 `mock.css` 一一对应，实现端（`src/theme.h`、`src/fonts.cpp`）直接照抄，避免稿子与产品漂移。

## 字体

| 用途 | 字体栈 | 字重 |
| --- | --- | --- |
| 全部 UI | `MiSans VF` → `MiSans` → `Noto Sans SC` → `HarmonyOS Sans SC` → `Microsoft YaHei UI` → `Segoe UI` | 400 / 500 / 600 |
| 渲染 | DirectWrite（灰度抗锯齿，字重映射 400→450 实例、500→520、600→630） | |

| 角色 | 字号 (DIP) | 行高 | 字重 |
| --- | --- | --- | --- |
| 题干 stem | 15 | 25.5 | 400 |
| 选项 opt | 13.5 | 21.6 | 400（正确项 600） |
| 元信息 idx/type | 11.5 | 16 | 400 |
| 关键词 chip | 11.5 | 16 | 500 |
| 答案 pill | 12 | 16 | 600 |
| 筛选 chip | 12.5 | 18 | 400（选中 600） |
| 结果统计 | 12 | 16 | 400（数字 500） |
| 空状态标题 / 说明 | 14 / 12.5 | 22 / 19 | 600 / 400 |

## 颜色

| Token | 浅色 | 深色 |
| --- | --- | --- |
| window | #F5F6F8 | #111317 |
| surface | #FFFFFF | #191C21 |
| surface-hover | #F7F8FA | #1E2228 |
| surface-sunken | #EFF1F4 | #1D2127 |
| border | #E4E7EC | #282C33 |
| border-strong | #DCE0E6 | #343941 |
| divider | #EEF0F3 | #24282F |
| text-primary | #14161A | #E8EAEE |
| text-secondary | #5B6270 | #A0A7B3 |
| text-muted | #98A0AD | #6C7480 |
| accent | #3D5BD9 | #95A6FF |
| accent-soft | #EDF0FC | #242B45 |
| success / soft | #0C7A55 / #E9F7F1 | #5FD8A4 / #153328 |
| danger / soft | #B92B2B / #FBEDED | #F08A8A / #3A1D1E |
| dot: judge / single / multi | #12A06C / #4E86E6 / #8F76DD | #35C08C / #7EA6F0 / #B394F0 |
| scroll-thumb | #C9CDD4 | #3A3F48 |

## 圆角与间距

| Token | 值 |
| --- | --- |
| radius.panel | 14 |
| radius.search | 10 |
| radius.chip | 8 |
| radius.pill | 6 |
| radius.tag | 4 |
| space.item.pad | 18 × 20 |
| gap.stem→meta | 10 |
| gap.opt 区块 | 8（行间 3） |
| gap.meta-right | 8 |
| scrollbar | 宽 6，右 6，上下内缩 14 |
| FAB | 32×32，右/下边距 24 |
