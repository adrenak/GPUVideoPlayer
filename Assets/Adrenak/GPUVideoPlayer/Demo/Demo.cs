using UnityEngine;
using UnityEngine.UI;

namespace Adrenak.GPUVideoPlayer.Demo {
	public class Demo : MonoBehaviour {
		public GPUVideoPlayer player;
		public Text message;
		public RawImage display;
		public Slider seekbar;

		bool isPlaying;

		private void Start() {
			player.onStateChanged.AddListener(state => {
				switch (state) {
					case GPUVideoPlayer.State.Paused:
						message.text = ("Paused");
						break;
					case GPUVideoPlayer.State.Playing:
						message.text = ("Playing");
						isPlaying = true;
						display.texture = player.MediaTexture;
						break;
					case GPUVideoPlayer.State.Stopped:
						message.text = ("Stopped");
						isPlaying = false;
						display.texture = null;
						break;
					case GPUVideoPlayer.State.Ended:
						message.text = ("Ended");
						isPlaying = false;
						break;
					case GPUVideoPlayer.State.Loaded:
						message.text = ("Loaded");
						var ratio = (float)player.MediaDescription.width / player.MediaDescription.height;
						display.GetComponent<AspectRatioFitter>().aspectRatio = ratio;
						break;
					case GPUVideoPlayer.State.Failed:
						message.text = ("Could not load");
						isPlaying = false;
						break;
				}
			});
		}

		void Update() {
			if (isPlaying)
				seekbar.value = (float)player.GetPosition() / player.GetDuration();
		}

		public void Load() {
			player.Load(Application.streamingAssetsPath + "/Adrenak/GPUVideoPlayer/test.mp4");
		}

		public void Pause() {
			player.Pause();
		}

		public void Stop() {
			player.Stop();
		}

		public void Play() {
			player.Play();
		}

		public void Seek(float ratio) {
			player.SeekByRatio(ratio);
		}

		public void MoveForwardFiveSeconds() {
			var position = player.GetPosition();
			player.SeekByTime(position + 5 * 10000000);
		}

        public void SetPlaybackRate(float rate)
        {
            player.SetPlaybackRate(rate);
        }

		public void MoveBackwardFiveSeconds() {
			var position = player.GetPosition();
			player.SeekByTime(position - 5 * 10000000);
		}
	}
}
