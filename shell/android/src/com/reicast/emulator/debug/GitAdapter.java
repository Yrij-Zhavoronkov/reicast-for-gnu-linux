package com.reicast.emulator.debug;

import java.util.ArrayList;
import java.util.HashMap;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.PorterDuff;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.WebSettings;
import android.webkit.WebSettings.PluginState;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.reicast.emulator.R;

public class GitAdapter extends BaseAdapter {

	private static Activity activity;
	private ArrayList<HashMap<String, String>> data;
	private LayoutInflater inflater = null;
	private DisplayImageOptions options;

	public GitAdapter(Activity a, ArrayList<HashMap<String, String>> d) {
		activity = a;
		this.data = d;
		this.inflater = (LayoutInflater) activity
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		ImageLoaderConfiguration configicon = new ImageLoaderConfiguration.Builder(
				activity).memoryCacheExtraOptions(96, 96).build();
		this.options = new DisplayImageOptions.Builder()
				.showStubImage(R.drawable.ic_github)
				.showImageForEmptyUri(R.drawable.ic_github)
				.imageScaleType(ImageScaleType.EXACTLY_STRETCHED).build();

		ImageLoader.getInstance().init(configicon);

	}

	public int getCount() {
		return this.data.size();
	}

	public Object getItem(int position) {
		return position;
	}

	public long getItemId(int position) {
		return position;
	}

	public View getView(final int position, View convertView, ViewGroup parent) {
		View vi = convertView;
		if (convertView == null)
			vi = this.inflater.inflate(R.layout.change_item, null);
		TextView dateText = (TextView) vi.findViewById(R.id.date);
		TextView committerText = (TextView) vi.findViewById(R.id.committer);
		TextView titleText = (TextView) vi.findViewById(R.id.title);
		ImageView avatarIcon = (ImageView) vi.findViewById(R.id.avatar);

		final HashMap<String, String> commit = this.data.get(position);
		final String date = commit.get("Date");
		final String committer = commit.get("Committer");
		final String title = commit.get("Title");
		final String message = commit.get("Message");
		final String sha = commit.get("Sha");
		final String url = commit.get("Url");
		final String author = commit.get("Author");
		final String avatar = commit.get("Avatar");
		final String current = commit.get("Build");

		RelativeLayout item = (RelativeLayout) vi.findViewById(R.id.change);
		if (current != null && !current.equals("") && current.equals(sha)) {
			item.getBackground().setColorFilter(0xFF00FF00,
					PorterDuff.Mode.MULTIPLY);
		} else {
			item.getBackground().setColorFilter(null);
		}

		dateText.setText(date);
		committerText.setText(committer);
		titleText.setText(title);
		ImageLoader.getInstance()
				.displayImage(avatar, avatarIcon, this.options);

		vi.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				System.gc();
				String output = title + "\n\n" + message + "\n\n" + " - " + author;
				displayCommit(sha, output, url, v.getContext());
			}
		});
		// Handle clicking individual item from list

		return vi;
	}

	private void displayCommit(final String sha, String message, String url,
			Context context) {
		final AlertDialog.Builder builder = new AlertDialog.Builder(context);
		builder.setCancelable(true);
		builder.setTitle(sha.substring(0,7));
		builder.setMessage(message);
		LayoutInflater infalter = LayoutInflater.from(context);
		final View popWebView = infalter.inflate(R.layout.webview, null);
		WebView mWebView = (WebView) popWebView.findViewById(R.id.webframe);
		mWebView = configureWebview(url, context, mWebView);
		builder.setView(popWebView);
		builder.setPositiveButton("Close",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						dialog.dismiss();
						return;
					}
				});
		builder.create().show();
	}

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@SuppressLint("SetJavaScriptEnabled")
	@SuppressWarnings("deprecation")
	private WebView configureWebview(String url, Context context,
			WebView mWebView) {
		mWebView.getSettings().setSupportZoom(true);
		mWebView.getSettings().setBuiltInZoomControls(true);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			mWebView.getSettings().setDisplayZoomControls(false);
		}
		mWebView.setInitialScale(1);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR) {
			mWebView.getSettings().setUseWideViewPort(true);
		}
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR_MR1) {
			mWebView.getSettings().setLoadWithOverviewMode(true);
		}
		mWebView.getSettings().setJavaScriptEnabled(true);
		mWebView.getSettings().setPluginState(PluginState.ON);
		mWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
		mWebView.clearHistory();
		mWebView.clearFormData();
		mWebView.clearCache(true);
		CookieSyncManager.createInstance(context);
		CookieManager cookieManager = CookieManager.getInstance();
		cookieManager.removeAllCookie();
		CookieSyncManager.getInstance().stopSync();
		mWebView.setWebViewClient(new WebViewClient() {
			@Override
			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				view.loadUrl(url);
				return true;
			}
		});
		mWebView.loadUrl(url);
		return mWebView;
	}
}