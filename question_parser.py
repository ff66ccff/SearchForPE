# -*- coding: utf-8 -*-
"""
题库解析模块
用于从docx文件中解析题目和答案
"""

from docx import Document
import re
import json
import os

class Question:
    """题目类"""
    def __init__(self, question_text, options=None, user_answer=None, correct_answer=None, question_type="判断题"):
        self.question_text = question_text
        self.options = options or []  # 选择题的选项
        self.user_answer = user_answer
        self.correct_answer = correct_answer
        self.question_type = question_type
    
    def to_dict(self):
        return {
            "question_text": self.question_text,
            "options": self.options,
            "user_answer": self.user_answer,
            "correct_answer": self.correct_answer,
            "question_type": self.question_type
        }
    
    @classmethod
    def from_dict(cls, data):
        return cls(
            question_text=data["question_text"],
            options=data.get("options", []),
            user_answer=data.get("user_answer"),
            correct_answer=data.get("correct_answer"),
            question_type=data.get("question_type", "判断题")
        )
    
    def get_display_text(self):
        """获取用于显示的完整题目文本"""
        text = f"【{self.question_type}】{self.question_text}\n"
        if self.options:
            text += "\n".join(self.options) + "\n"
        if self.correct_answer:
            text += f"\n✅ 正确答案：{self.correct_answer}"
        return text


def parse_docx(docx_path, progress_callback=None):
    """
    解析docx文件中的所有题目
    
    Args:
        docx_path: docx文件路径
        progress_callback: 进度回调函数，接收(当前进度, 总进度)参数
    
    Returns:
        题目列表
    """
    doc = Document(docx_path)
    paragraphs = [p.text.strip() for p in doc.paragraphs]
    total = len(paragraphs)
    
    questions = []
    i = 0
    
    while i < total:
        if progress_callback and i % 100 == 0:
            progress_callback(i, total)
        
        text = paragraphs[i]
        
        if not text:
            i += 1
            continue
        
        # 跳过答案行，这些会在下面处理
        if text.startswith("你的答案") or text.startswith("你未作答"):
            i += 1
            continue
        
        # 检查是否是选项行
        if re.match(r'^[A-Z][．\.\、]', text):
            i += 1
            continue
        
        # 检查是否是题目开始
        # 题目特征：不以选项格式开始，不是答案行，且下一行或几行后有答案行
        is_question = False
        look_ahead = min(i + 15, total)  # 最多往后看15行
        
        for j in range(i + 1, look_ahead):
            if j < total and (paragraphs[j].startswith("你的答案") or paragraphs[j].startswith("你未作答")):
                is_question = True
                break
        
        if is_question:
            question_text = text
            options = []
            user_answer = None
            correct_answer = None
            question_type = "判断题"
            
            i += 1
            
            # 收集选项
            while i < total:
                current = paragraphs[i]
                if not current:
                    i += 1
                    continue
                
                # 检查是否是选项
                if re.match(r'^[A-Z][．\.\、]', current):
                    options.append(current)
                    question_type = "选择题"
                    i += 1
                elif current.startswith("你的答案") or current.startswith("你未作答"):
                    # 解析答案行
                    answer_text = current
                    
                    # 解析用户答案和标准答案
                    if "标准答案：" in answer_text:
                        parts = answer_text.split("标准答案：")
                        if parts[0].startswith("你的答案："):
                            user_answer = parts[0].replace("你的答案：", "").strip()
                        elif parts[0].startswith("你未作答"):
                            user_answer = "未作答"
                        correct_answer = parts[1].strip()
                    else:
                        if answer_text.startswith("你的答案："):
                            user_answer = answer_text.replace("你的答案：", "").strip()
                            correct_answer = user_answer  # 如果没有标准答案，说明答对了
                        elif answer_text.startswith("你未作答"):
                            user_answer = "未作答"
                    
                    i += 1
                    break
                else:
                    # 可能是题目的续行
                    if not options and not current.startswith("你"):
                        question_text += current
                    i += 1
                    break
            
            # 创建题目对象
            question = Question(
                question_text=question_text,
                options=options,
                user_answer=user_answer,
                correct_answer=correct_answer,
                question_type=question_type
            )
            questions.append(question)
        else:
            i += 1
    
    if progress_callback:
        progress_callback(total, total)
    
    return questions


def save_questions_to_json(questions, json_path):
    """将题目保存到JSON文件"""
    data = [q.to_dict() for q in questions]
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def load_questions_from_json(json_path):
    """从JSON文件加载题目"""
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    return [Question.from_dict(d) for d in data]


def search_questions(questions, keyword, search_in_options=True):
    """
    搜索题目
    
    Args:
        questions: 题目列表
        keyword: 关键词
        search_in_options: 是否在选项中搜索
    
    Returns:
        匹配的题目列表
    """
    keyword = keyword.lower()
    results = []
    
    for q in questions:
        if keyword in q.question_text.lower():
            results.append(q)
        elif search_in_options and q.options:
            for opt in q.options:
                if keyword in opt.lower():
                    results.append(q)
                    break
    
    return results


if __name__ == "__main__":
    # 测试解析
    docx_path = r"d:\SearchForPE\体育理论考试总题库.docx"
    json_path = r"d:\SearchForPE\questions.json"
    
    print("正在解析题库...")
    questions = parse_docx(docx_path)
    print(f"共解析出 {len(questions)} 道题目")
    
    # 保存到JSON
    save_questions_to_json(questions, json_path)
    print(f"已保存到 {json_path}")
    
    # 显示前5道题
    for i, q in enumerate(questions[:5]):
        print(f"\n--- 题目 {i+1} ---")
        print(q.get_display_text())
