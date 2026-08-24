const fs = require('fs');
const path = require('path');

const EXCLUDE_DIRS = [
  '.git',
  'build',
  'dist',
  'out',
  '.idea',
  '.vscode',
  '__pycache__',
  '.history',
];

function shouldExclude(item) {
  if (item.startsWith('.')) return true;
  if (EXCLUDE_DIRS.includes(item)) return true;
  return false;
}

// ========== 递归获取所有子文件夹 ==========
function getAllDirs(dir) {
  const results = [];
  
  if (!fs.existsSync(dir)) return results;

  const list = fs.readdirSync(dir);
  list.forEach((item) => {
    const fullPath = path.join(dir, item);
    
    if (fs.statSync(fullPath).isDirectory()) {
      if (!shouldExclude(item)) {
        const relative = path.relative(__dirname, fullPath);
        results.push(relative);
        results.push(...getAllDirs(fullPath));
      }
    }
  });
  
  return results;
}

module.exports = {
  // ==================== 核心行为配置 ====================

  // 【1】允许自定义范围（scope）—— 如果设为 false，则只能从 scopes 列表中选择
  allowCustomScopes: true,

  // 【2】预设的范围列表 —— 会被显示为可选项，支持方向键选择或输入搜索
  scopes: getAllDirs(__dirname), // 使用文件夹名称作为范围列表

  // 【3】是否开启范围（scope）多选 —— 开启后，选择范围时可用空格键复选
  enableMultipleScopes: true,

  // 【4】默认范围（scope）—— 每次启动时预填的默认值，可被用户覆盖
  defaultScope: '',

  // ==================== 自定义提交类型（type） ====================

  // 【5】自定义提交类型列表 —— 覆盖默认的 type 选项
  // 如果不定义，会使用内置的 Angular 风格类型
  types: [
    { value: 'feat',     name: '✨ feat:     新功能' },
    { value: 'fix',      name: '🐛 fix:      修复 Bug' },
    { value: 'docs',     name: '📝 docs:     文档变更' },
    { value: 'style',    name: '💄 style:    代码格式（不影响代码运行）' },
    { value: 'refactor', name: '♻️ refactor: 重构（既不是新功能，也不是修复）' },
    { value: 'perf',     name: '⚡ perf:     性能优化' },
    { value: 'test',     name: '✅ test:     添加或修改测试' },
    { value: 'build',    name: '📦 build:    构建系统或外部依赖变更' },
    { value: 'ci',       name: '🔧 ci:       CI 配置变更' },
    { value: 'chore',    name: '🧹 chore:    其他杂项（不修改 src 或 test）' },
    { value: 'revert',   name: '⏪ revert:   回退之前的提交' }
  ],

  // ==================== 自定义交互提示语 ====================

  // 【6】自定义每个步骤的提示信息（可以改成中文）
  messages: {
    type: '选择你本次提交的类型：',
    scope: '表示本次变更的影响范围（可选）：',
    customScope: '请输入自定义的范围：',
    subject: '请用一句话简要描述本次变更（必填）：\n',
    body: '请输入详细的变更说明（可选，按回车跳过）：\n',
    breaking: '是否有破坏性变更？（默认否，输入 y/n）\n',
    footer: '关联的 Issue 编号，例如 "fix #123"（可选，按回车跳过）：\n',
    confirmCommit: '确认提交以上信息吗？（y/n）\n'
  },

  // ==================== 高级选项 ====================

  // 【7】是否允许跳过必填项（如 subject）—— 不推荐开启，设为 false 强制填写
  allowEmptySubject: false,

  // 【8】是否在提交前自动带上 Emoji（需配合 types 中的 Emoji 使用）
  useEmoji: true,

  // 【9】主题（subject）的最大长度（默认 100）
  subjectLimit: 100,

  // 【10】是否要强制填写 body（详细说明）
  requireBody: false,

  // 【11】是否要强制填写 footer（关联 issue）
  requireFooter: false,

  // 【12】是否在提交前显示最终信息预览并确认（默认 true）
  confirmCommit: true,

  // 【13】针对某些类型跳过 scope 步骤（例如 revert 不需要 scope）
  skipScopes: ['revert'],

  // 【14】是否在提交前检查是否有暂存文件（默认为 true，若无暂存则提示）
  checkStaged: true,

  // ==================== AI 辅助相关（czg ai） ====================

  // 【15】AI 生成配置（需要配置 OpenAI API Key 等）
  // 如果你不使用 czg ai，可以忽略此项
  // ai: {
  //   apiKey: process.env.OPENAI_API_KEY,
  //   model: 'gpt-3.5-turbo',
  //   questionCB: ({ diff }) => `Please generate a commit message in English for the following diff:\n\n${diff}`,
  // },
};