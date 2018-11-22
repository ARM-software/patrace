package com.arm.pa.paretrace.Activities;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.HorizontalScrollView;
import android.content.res.TypedArray;
import android.view.MotionEvent;

import com.arm.pa.paretrace.R;

public class HorizontalScrollViewNoHover extends HorizontalScrollView {
    public HorizontalScrollViewNoHover (Context context)
    {
        super(context);
    }
    public HorizontalScrollViewNoHover (Context context,
                AttributeSet attrs)

    {
        super(context, attrs);
        initViews(context, attrs);
    }
    public HorizontalScrollViewNoHover (Context context,
                AttributeSet attrs,
                int defStyleAttr)

    {
        super(context, attrs, defStyleAttr);
        initViews(context, attrs);
    }
    public HorizontalScrollViewNoHover (Context context,
                AttributeSet attrs,
                int defStyleAttr,
                int defStyleRes)

    {
        super(context, attrs, defStyleAttr, defStyleRes);
        initViews(context, attrs);
    }

    private void initViews(Context context, AttributeSet attrs) {
        TypedArray a = context.getTheme().obtainStyledAttributes(attrs, R.styleable.HorizontalScrollViewNoHover, 0, 0);
        a.recycle();
    }

    @Override
    public boolean onHoverEvent (MotionEvent event) {
        setHorizontalScrollBarEnabled(false);
        return false;
    }

    @Override
    public void onScrollChanged (int l, int t, int oldl, int oldt) {
        setHorizontalScrollBarEnabled(true);
    }
}
