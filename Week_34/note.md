# 第 3 周学习笔记

## 语法知识

**vector用法**
```
vec.push_back()         //尾部添加元素
vec.erase()             //删除元素
vec.insert()            //头部插入元素

vec.shrink_to_fit()     //先清空元素
vec.shrink_to_fit       //再释放内存
```

**Markdown**
```
# 一级标题
## 二级标题
** 加粗 **
* 斜体 *
- 列表
> 引用块
>> 嵌套引用块
[链接名称](链接地址)
```

## 自瞄算法

**统一旋转矩形长宽**
```
...
float w = rect.size.width;
            float h = rect.size.height;
            if (w == 0 || h == 0 ) continue;
            if (w > h)
            {
                flag = 1;
                float temp = w;
                w = h;
                h = temp;
            }
...
```

**根据长宽情况分类判断倾角**
```
...
if (flag == 0 && rect.angle > 30) continue;
if (flag == 1 && rect.angle < 60) {flag = 0; continue;}
...
```

**遍历轮廓组合,排除孤立离群和距离过近**
```
...
for (size_t j = 0; j < lightBars.size(); ++j)
        {
            bool hasNeighbor = false;
            for (size_t k = 0; k < lightBars.size(); ++k)
            {
                if (j == k) continue;
                
                float centerDistance = cv::norm(lightBars[j].center - lightBars[k].center);
                float averageHeight = (lightBars[j].height + lightBars[k].height) * 0.5f;
                if (centerDistance <= 5.0f * averageHeight && centerDistance >= 0.8f * averageHeight)
                {
                    hasNeighbor = true;
                    break;
                }
            }

            if (hasNeighbor)
            {
                pairedLightBars.push_back(lightBars[j]);
            }
        }
...
```
