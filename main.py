# -*- coding: utf-8 -*-
"""
体育理论题库查询系统
一个优美的图形界面应用，用于快速查询题目
"""

import customtkinter as ctk
from tkinter import messagebox
import json
import os
import sys
import threading

# 设置外观模式和主题
ctk.set_appearance_mode("light")  # 可选: "light", "dark", "system"
ctk.set_default_color_theme("blue")  # 可选: "blue", "green", "dark-blue"


def get_resource_path(filename):
    """获取资源文件路径，支持打包后的exe"""
    if getattr(sys, 'frozen', False):
        # 打包后的exe
        base_path = sys._MEIPASS
    else:
        # 开发环境
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, filename)


class Question:
    """题目类 - 使用 __slots__ 减少内存占用"""
    __slots__ = ['question_text', 'options', 'correct_answer', 'question_type', 
                 'search_text', 'search_options_text']
    
    def __init__(self, question_text, options=None, user_answer=None, correct_answer=None, question_type="判断题"):
        self.question_text = question_text
        self.options = options or []
        self.correct_answer = correct_answer
        self.question_type = question_type
        # 预处理搜索文本，转为小写存储
        self.search_text = question_text.lower()
        self.search_options_text = ' '.join(options).lower() if options else ''
    
    @classmethod
    def from_dict(cls, data):
        return cls(
            question_text=data["question_text"],
            options=data.get("options", []),
            user_answer=data.get("user_answer"),
            correct_answer=data.get("correct_answer"),
            question_type=data.get("question_type", "判断题")
        )


class QuestionCard(ctk.CTkFrame):
    """题目卡片组件"""
    def __init__(self, master, question, index, **kwargs):
        super().__init__(master, **kwargs)
        self.question = question
        
        # 配置卡片样式
        self.configure(
            corner_radius=12,
            fg_color=("white", "#2b2b2b"),
            border_width=1,
            border_color=("#e0e0e0", "#404040")
        )
        
        # 题目序号和类型标签
        header_frame = ctk.CTkFrame(self, fg_color="transparent")
        header_frame.pack(fill="x", padx=15, pady=(12, 5))
        
        # 类型标签
        type_color = "#4CAF50" if question.question_type == "判断题" else "#2196F3"
        type_label = ctk.CTkLabel(
            header_frame,
            text=f" {question.question_type} ",
            font=ctk.CTkFont(size=12, weight="bold"),
            fg_color=type_color,
            corner_radius=4,
            text_color="white"
        )
        type_label.pack(side="left")
        
        # 题目序号
        index_label = ctk.CTkLabel(
            header_frame,
            text=f"  #{index}",
            font=ctk.CTkFont(size=12),
            text_color=("gray50", "gray60")
        )
        index_label.pack(side="left")
        
        # 题目内容
        question_label = ctk.CTkLabel(
            self,
            text=question.question_text,
            font=ctk.CTkFont(size=14),
            wraplength=700,
            justify="left",
            anchor="w"
        )
        question_label.pack(fill="x", padx=15, pady=5)
        
        # 选项（如果有）
        if question.options:
            options_frame = ctk.CTkFrame(self, fg_color=("#f5f5f5", "#363636"), corner_radius=8)
            options_frame.pack(fill="x", padx=15, pady=5)
            
            for opt in question.options:
                opt_label = ctk.CTkLabel(
                    options_frame,
                    text=opt,
                    font=ctk.CTkFont(size=13),
                    anchor="w",
                    justify="left"
                )
                opt_label.pack(fill="x", padx=10, pady=2)
        
        # 答案区域
        answer_frame = ctk.CTkFrame(self, fg_color=("#e8f5e9", "#1b5e20"), corner_radius=8)
        answer_frame.pack(fill="x", padx=15, pady=(5, 12))
        
        answer_text = f"✅ 正确答案：{question.correct_answer}" if question.correct_answer else "❓ 答案未知"
        answer_label = ctk.CTkLabel(
            answer_frame,
            text=answer_text,
            font=ctk.CTkFont(size=14, weight="bold"),
            text_color=("#2e7d32", "#81c784"),
            anchor="w"
        )
        answer_label.pack(fill="x", padx=12, pady=8)


class SearchApp(ctk.CTk):
    """主应用程序"""
    def __init__(self):
        super().__init__()
        
        # 窗口配置
        self.title("🏃 体育理论题库查询系统")
        self.geometry("900x700")
        self.minsize(800, 600)
        
        # 数据
        self.questions = []
        self.judge_questions = []  # 预分类：判断题
        self.choice_questions = []  # 预分类：选择题
        self.search_results = []
        self.current_page = 0
        self.page_size = 20
        
        # 创建界面
        self._create_ui()
        
        # 加载题库
        self.after(100, self._load_questions)
    
    def _create_ui(self):
        """创建用户界面"""
        # 主容器
        self.main_container = ctk.CTkFrame(self, fg_color="transparent")
        self.main_container.pack(fill="both", expand=True, padx=20, pady=20)
        
        # ===== 顶部区域 =====
        self._create_header()
        
        # ===== 搜索区域 =====
        self._create_search_area()
        
        # ===== 结果区域 =====
        self._create_results_area()
        
        # ===== 底部分页区域 =====
        self._create_pagination()
        
        # ===== 状态栏 =====
        self._create_status_bar()
    
    def _create_header(self):
        """创建顶部标题"""
        header_frame = ctk.CTkFrame(self.main_container, fg_color="transparent")
        header_frame.pack(fill="x", pady=(0, 15))
        
        # 标题
        title_label = ctk.CTkLabel(
            header_frame,
            text="🏃 体育理论题库查询系统",
            font=ctk.CTkFont(size=28, weight="bold")
        )
        title_label.pack(side="left")
        
        # 主题切换按钮
        self.theme_button = ctk.CTkButton(
            header_frame,
            text="🌙",
            width=40,
            height=40,
            corner_radius=20,
            command=self._toggle_theme,
            fg_color=("gray85", "gray25"),
            hover_color=("gray75", "gray35"),
            text_color=("gray20", "gray90")
        )
        self.theme_button.pack(side="right")
    
    def _create_search_area(self):
        """创建搜索区域"""
        search_frame = ctk.CTkFrame(
            self.main_container,
            corner_radius=15,
            fg_color=("#f0f4f8", "#1e1e1e")
        )
        search_frame.pack(fill="x", pady=(0, 15))
        
        # 搜索框容器
        search_inner = ctk.CTkFrame(search_frame, fg_color="transparent")
        search_inner.pack(fill="x", padx=20, pady=20)
        
        # 搜索图标和输入框
        self.search_entry = ctk.CTkEntry(
            search_inner,
            placeholder_text="🔍 输入关键词搜索题目...",
            font=ctk.CTkFont(size=16),
            height=50,
            corner_radius=25
        )
        self.search_entry.pack(side="left", fill="x", expand=True, padx=(0, 10))
        self.search_entry.bind("<Return>", lambda e: self._search())
        
        # 清除按钮
        self.clear_button = ctk.CTkButton(
            search_inner,
            text="✕",
            font=ctk.CTkFont(size=16),
            width=50,
            height=50,
            corner_radius=25,
            fg_color=("gray80", "gray30"),
            hover_color=("gray70", "gray40"),
            text_color=("gray30", "gray80"),
            command=self._clear_search
        )
        self.clear_button.pack(side="left", padx=(0, 10))
        
        # 搜索按钮
        self.search_button = ctk.CTkButton(
            search_inner,
            text="搜索",
            font=ctk.CTkFont(size=16, weight="bold"),
            width=100,
            height=50,
            corner_radius=25,
            command=self._search
        )
        self.search_button.pack(side="left")
        
        # 筛选选项
        filter_frame = ctk.CTkFrame(search_frame, fg_color="transparent")
        filter_frame.pack(fill="x", padx=20, pady=(0, 15))
        
        # 题目类型筛选
        ctk.CTkLabel(
            filter_frame,
            text="题目类型：",
            font=ctk.CTkFont(size=13)
        ).pack(side="left")
        
        self.type_var = ctk.StringVar(value="全部")
        type_menu = ctk.CTkOptionMenu(
            filter_frame,
            values=["全部", "判断题", "选择题"],
            variable=self.type_var,
            width=100,
            height=30,
            corner_radius=8
        )
        type_menu.pack(side="left", padx=(5, 20))
        
        # 搜索范围
        self.search_in_options = ctk.CTkCheckBox(
            filter_frame,
            text="同时搜索选项内容",
            font=ctk.CTkFont(size=13)
        )
        self.search_in_options.pack(side="left")
        self.search_in_options.select()  # 默认选中
    
    def _create_results_area(self):
        """创建结果显示区域"""
        # 结果统计
        self.result_info = ctk.CTkLabel(
            self.main_container,
            text="📚 题库加载中...",
            font=ctk.CTkFont(size=14),
            anchor="w"
        )
        self.result_info.pack(fill="x", pady=(0, 10))
        
        # 可滚动的结果区域
        self.results_scroll = ctk.CTkScrollableFrame(
            self.main_container,
            corner_radius=10,
            fg_color="transparent"
        )
        self.results_scroll.pack(fill="both", expand=True)
    
    def _create_pagination(self):
        """创建分页控件"""
        self.pagination_frame = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.pagination_frame.pack(fill="x", pady=(15, 0))
        
        # 上一页按钮
        self.prev_button = ctk.CTkButton(
            self.pagination_frame,
            text="◀ 上一页",
            width=100,
            height=35,
            corner_radius=8,
            command=self._prev_page,
            state="disabled"
        )
        self.prev_button.pack(side="left")
        
        # 页码信息
        self.page_label = ctk.CTkLabel(
            self.pagination_frame,
            text="第 0 / 0 页",
            font=ctk.CTkFont(size=13)
        )
        self.page_label.pack(side="left", expand=True)
        
        # 下一页按钮
        self.next_button = ctk.CTkButton(
            self.pagination_frame,
            text="下一页 ▶",
            width=100,
            height=35,
            corner_radius=8,
            command=self._next_page,
            state="disabled"
        )
        self.next_button.pack(side="right")
    
    def _create_status_bar(self):
        """创建状态栏"""
        self.status_bar = ctk.CTkLabel(
            self,
            text="就绪",
            font=ctk.CTkFont(size=12),
            fg_color=("#e3f2fd", "#1a237e"),
            corner_radius=0,
            height=30
        )
        self.status_bar.pack(fill="x", side="bottom")
    
    def _toggle_theme(self):
        """切换主题"""
        current = ctk.get_appearance_mode()
        if current == "Light":
            ctk.set_appearance_mode("dark")
            self.theme_button.configure(text="☀️")
        else:
            ctk.set_appearance_mode("light")
            self.theme_button.configure(text="🌙")
    
    def _load_questions(self):
        """加载题库"""
        self._set_status("正在加载题库...")
        
        def load():
            try:
                json_path = get_resource_path("questions.json")
                
                if os.path.exists(json_path):
                    with open(json_path, 'r', encoding='utf-8') as f:
                        data = json.load(f)
                    
                    # 批量创建对象并预分类
                    questions = []
                    judge_questions = []
                    choice_questions = []
                    
                    for d in data:
                        q = Question.from_dict(d)
                        questions.append(q)
                        if q.question_type == "判断题":
                            judge_questions.append(q)
                        else:
                            choice_questions.append(q)
                    
                    self.questions = questions
                    self.judge_questions = judge_questions
                    self.choice_questions = choice_questions
                    
                    self.after(0, lambda: self._on_questions_loaded())
                else:
                    self.after(0, lambda: self._show_error("找不到题库文件 questions.json"))
            except Exception as e:
                self.after(0, lambda: self._show_error(f"加载题库失败: {str(e)}"))
        
        threading.Thread(target=load, daemon=True).start()
    
    def _on_questions_loaded(self):
        """题库加载完成"""
        count = len(self.questions)
        judge_count = sum(1 for q in self.questions if q.question_type == "判断题")
        choice_count = count - judge_count
        
        self.result_info.configure(
            text=f"📚 题库已加载：共 {count:,} 道题目（判断题 {judge_count:,} 道，选择题 {choice_count:,} 道）"
        )
        self._set_status(f"题库加载完成，共 {count:,} 道题目")
    
    def _clear_search(self):
        """清除搜索框和结果"""
        self.search_entry.delete(0, "end")
        self.search_results = []
        self.current_page = 0
        
        # 清空结果区域
        for widget in self.results_scroll.winfo_children():
            widget.destroy()
        
        # 恢复提示信息
        count = len(self.questions)
        judge_count = len(self.judge_questions)
        choice_count = len(self.choice_questions)
        self.result_info.configure(
            text=f"📚 题库已加载：共 {count:,} 道题目（判断题 {judge_count:,} 道，选择题 {choice_count:,} 道）"
        )
        self._update_pagination()
        self._set_status("已清除搜索")
        
        # 聚焦到搜索框
        self.search_entry.focus()
    
    def _search(self):
        """执行搜索 - 优化版本"""
        keyword = self.search_entry.get().strip()
        if not keyword:
            messagebox.showwarning("提示", "请输入搜索关键词")
            return
        
        self._set_status(f"正在搜索：{keyword}...")
        self.search_button.configure(state="disabled")
        
        def do_search():
            keyword_lower = keyword.lower()
            type_filter = self.type_var.get()
            search_options = self.search_in_options.get()
            
            # 根据类型筛选选择搜索范围
            if type_filter == "判断题":
                search_pool = self.judge_questions
            elif type_filter == "选择题":
                search_pool = self.choice_questions
            else:
                search_pool = self.questions
            
            # 使用列表推导式加速搜索
            if search_options:
                results = [q for q in search_pool 
                          if keyword_lower in q.search_text or keyword_lower in q.search_options_text]
            else:
                results = [q for q in search_pool if keyword_lower in q.search_text]
            
            self.after(0, lambda: self._on_search_complete(results, keyword))
        
        threading.Thread(target=do_search, daemon=True).start()
    
    def _on_search_complete(self, results, keyword):
        """搜索完成回调"""
        self.search_results = results
        self.current_page = 0
        self._display_results()
        self.search_button.configure(state="normal")
        self._set_status(f"搜索完成：找到 {len(results)} 道相关题目")
    
    def _display_results(self):
        """显示搜索结果"""
        # 清空现有结果
        for widget in self.results_scroll.winfo_children():
            widget.destroy()
        
        if not self.search_results:
            no_result = ctk.CTkLabel(
                self.results_scroll,
                text="😕 没有找到相关题目\n\n试试其他关键词吧",
                font=ctk.CTkFont(size=16),
                text_color="gray50"
            )
            no_result.pack(pady=50)
            self.result_info.configure(text="🔍 未找到相关题目")
            self._update_pagination()
            return
        
        # 计算分页
        total = len(self.search_results)
        start = self.current_page * self.page_size
        end = min(start + self.page_size, total)
        
        self.result_info.configure(
            text=f"🔍 找到 {total:,} 道相关题目，显示第 {start + 1} - {end} 道"
        )
        
        # 显示当前页的题目
        for i, q in enumerate(self.search_results[start:end], start=start + 1):
            card = QuestionCard(self.results_scroll, q, i)
            card.pack(fill="x", pady=5)
        
        self._update_pagination()
    
    def _update_pagination(self):
        """更新分页控件"""
        total_pages = max(1, (len(self.search_results) + self.page_size - 1) // self.page_size)
        current = self.current_page + 1
        
        self.page_label.configure(text=f"第 {current} / {total_pages} 页")
        
        # 更新按钮状态
        self.prev_button.configure(state="normal" if self.current_page > 0 else "disabled")
        self.next_button.configure(state="normal" if current < total_pages else "disabled")
    
    def _prev_page(self):
        """上一页"""
        if self.current_page > 0:
            self.current_page -= 1
            self._display_results()
            self.results_scroll._parent_canvas.yview_moveto(0)
    
    def _next_page(self):
        """下一页"""
        total_pages = (len(self.search_results) + self.page_size - 1) // self.page_size
        if self.current_page < total_pages - 1:
            self.current_page += 1
            self._display_results()
            self.results_scroll._parent_canvas.yview_moveto(0)
    
    def _set_status(self, text):
        """设置状态栏文本"""
        self.status_bar.configure(text=f"  {text}")
    
    def _show_error(self, message):
        """显示错误"""
        messagebox.showerror("错误", message)
        self._set_status("发生错误")


def main():
    app = SearchApp()
    app.mainloop()


if __name__ == "__main__":
    main()
