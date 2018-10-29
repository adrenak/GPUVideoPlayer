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
			player.OnPaused.AddListener(() => {
				message.text = ("Paused");
			});

			player.OnPlay.AddListener(() => {
				message.text = ("Playing");
				isPlaying = true;
				display.texture = player.MediaTexture;
			});

			player.OnStopped.AddListener(() => {
				message.text = ("Stopped");
				isPlaying = false;
				display.texture = null;
			});

			player.OnEnded.AddListener(() => {
				message.text = ("Ended");
				isPlaying = false;
			});

			player.OnLoaded.AddListener(() => {
				message.text = ("Loaded");
				var ratio = (float)player.MediaDescription.width / player.MediaDescription.height;
				display.GetComponent<AspectRatioFitter>().aspectRatio = ratio;
			});

			player.OnFailed.AddListener(() => {
				message.text = ("Could not load");
				isPlaying = false;
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

		public void MoveBackwardFiveSeconds() {
			var position = player.GetPosition();
			player.SeekByTime(position - 5 * 10000000);
		}
	}
}
