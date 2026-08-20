# 贡献流程

本项目使用 Forking Workflow。不要直接向源仓库的 `main` 分支提交修改。

## 1. Fork、Clone 并配置远程仓库

```bash
git clone git@github.com:<your-account>/sturdy-guide.git
cd sturdy-guide
git remote add upstream git@github.com:Teahp/sturdy-guide.git
git remote -v
```

- `origin`：自己的 Fork，可以推送。
- `upstream`：源仓库，用于同步项目进展。

## 2. 同步并创建功能分支

```bash
git switch main
git fetch upstream
git merge --ff-only upstream/main
git push origin main
git switch -c feature/short-description
```

分支名使用 `feature/...`、`fix/...` 或 `docs/...`。

## 3. 构建、测试并提交

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure

git status
git diff
git add <files>
git commit -m "feat: describe the change"
```

提交说明使用简短的 `feat:`、`fix:`、`docs:`、`test:` 或 `ci:` 前缀。

## 4. 推送并发起 PR

```bash
git push -u origin feature/short-description
```

从 `个人Fork:feature/short-description` 向 `Teahp/sturdy-guide:main` 发起 PR。填写修改内容、原因和验证方法，等待 CI 与代码评审。

评审要求修改时，继续向同一分支提交并推送；原 PR 会自动更新。CI 通过并获得批准后，由维护者按项目策略合并。

## 5. 合并后同步与清理

```bash
git switch main
git fetch upstream
git merge --ff-only upstream/main
git push origin main
git push origin --delete feature/short-description
git branch -d feature/short-description
```

